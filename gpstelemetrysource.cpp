/**
 * @file gpstelemetrysource.cpp
 * @brief Implémentation de la source GPS matérielle.
 * @details Responsabilités : Configurer le port série, décoder le flux NMEA en continu
 * et traduire les mesures brutes en télémétrie exploitable par l'interface graphique.
 * Dépendances principales : Qt SerialPort, Qt Positioning et TelemetryData.
 */

#include "gpstelemetrysource.h"
#include "telemetrydata.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>

GpsTelemetrySource::GpsTelemetrySource(TelemetryData* data, QObject* parent)
    : QObject(parent), m_data(data)
{
    // Initialisation de l'interface série matérielle
    m_serial = new QSerialPort(this);
}

GpsTelemetrySource::~GpsTelemetrySource() {
    stop();
}

void GpsTelemetrySource::start(const QString& portName) {
    // Redémarrage idempotent : on repart d'un état propre et on referme le port
    // s'il était déjà ouvert avant toute nouvelle tentative.
    stop();

    QString scriptPath = QCoreApplication::applicationDirPath() + "/scripts/agps_loader.py";
    if (!QFileInfo::exists(scriptPath)) {
        scriptPath = QDir::currentPath() + "/scripts/agps_loader.py";
    }

    if (QFileInfo::exists(scriptPath)) {
        qDebug() << "🚀 Lancement du script A-GPS AssistNow :" << scriptPath;

        QProcess agpsProcess;
        agpsProcess.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        agpsProcess.setProcessChannelMode(QProcess::MergedChannels);
        agpsProcess.setProgram("python3");
        agpsProcess.setArguments({scriptPath, portName});
        agpsProcess.start();

        if (!agpsProcess.waitForStarted(3000)) {
            qWarning() << "⚠️ Impossible de démarrer le script A-GPS :"
                       << agpsProcess.errorString();
        } else {
            const bool finished = agpsProcess.waitForFinished(15000);
            if (!finished) {
                qWarning() << "⚠️ Timeout (15s) pendant l'injection A-GPS."
                           << "Le port GPS ne sera pas ouvert.";
                agpsProcess.kill();
                agpsProcess.waitForFinished(2000);
                if (m_data) m_data->setGpsOk(false);
                return;
            }

            const QByteArray agpsOutput = agpsProcess.readAll();
            if (!agpsOutput.trimmed().isEmpty()) {
                qDebug().noquote() << "A-GPS output:\n" + QString::fromUtf8(agpsOutput);
            }

            if (agpsProcess.exitStatus() != QProcess::NormalExit || agpsProcess.exitCode() != 0) {
                qWarning() << "⚠️ Injection A-GPS terminée avec erreur. Code de sortie :"
                           << agpsProcess.exitCode();
            } else {
                qDebug() << "✅ Injection A-GPS terminée avec succès.";
            }
        }
    } else {
        qWarning() << "⚠️ Script A-GPS introuvable, démarrage GPS sans injection :" << scriptPath;
    }

    // Configuration de la connexion physique au module GPS (ex: NEO-6M)
    m_serial->setPortName(portName);
    m_serial->setBaudRate(QSerialPort::Baud9600); // 9600 bauds est le standard industriel NMEA par défaut

    if (!m_serial->open(QIODevice::ReadOnly)) {
        qCritical() << "❌ Erreur : Impossible d'ouvrir le module GPS sur le port" << portName;
        if(m_data) m_data->setGpsOk(false);
        return;
    }

    // Création du parseur NMEA en "RealTimeMode" (lit le flux en direct au lieu d'un fichier log)
    m_nmeaSource = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode, this);
    m_nmeaSource->setDevice(m_serial);

    // Connexion du moteur Qt Positioning à notre logique métier
    connect(m_nmeaSource, &QNmeaPositionInfoSource::positionUpdated,
            this, &GpsTelemetrySource::onPositionUpdated);

    // Démarrage de la boucle de lecture
    m_nmeaSource->startUpdates();

    qDebug() << "✅ GPS Démarré (Mode Qt Positioning) sur" << portName;
}

void GpsTelemetrySource::stop() {
    // L'arrêt explicite du parseur et la suppression de l'objet évitent
    // des callbacks fantômes lors des changements d'état de l'application.
    if (m_nmeaSource) {
        m_nmeaSource->stopUpdates();
        delete m_nmeaSource;
        m_nmeaSource = nullptr;
    }

    // Libération matérielle du port série
    if (m_serial->isOpen()) {
        m_serial->close();
    }
}

void GpsTelemetrySource::onPositionUpdated(const QGeoPositionInfo &info) {
    if (!m_data) return;

    if (info.isValid()) {
        // Le module GPS "fixe" les satellites (position 3D validée)
        m_data->setGpsOk(true);

        QGeoCoordinate coord = info.coordinate();
        m_data->setLat(coord.latitude());
        m_data->setLon(coord.longitude());

        // Extraction de la vitesse (si la trame NMEA RMC ou VTG la fournit)
        double speedMs = 0.0;
        if (info.hasAttribute(QGeoPositionInfo::GroundSpeed)) {
            speedMs = info.attribute(QGeoPositionInfo::GroundSpeed); // En mètres par seconde
            m_data->setSpeedKmh(speedMs * 3.6); // Conversion en km/h pour l'affichage tableau de bord
        }

        // Extraction du cap (Direction)
        if (info.hasAttribute(QGeoPositionInfo::Direction)) {
            double course = info.attribute(QGeoPositionInfo::Direction);

            // LOGIQUE MÉTIER CRITIQUE :
            // Sous une faible vitesse, le calcul de cap (Heading) par le GPS devient erratique
            // car le module ne peut plus déterminer l'avant de l'arrière.
            // On applique un seuil (3 km/h) pour éviter que la carte GPS ne pivote brutalement
            // dans tous les sens lorsque le véhicule est arrêté à un feu rouge.
            /*if (speedMs * 3.6 > 3.0) {
                m_data->setHeading(course);
            }*/
        }
    } else {
        // Le GPS est allumé mais cherche encore ses satellites (Cold/Warm start)
        m_data->setGpsOk(false);
        qDebug() << "GPS : En attente de satellites (No Fix)...";
    }
}
