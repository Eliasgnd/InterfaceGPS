/**
 * @file clavier.h
 * @brief Rôle architectural : Clavier virtuel propriétaire utilisé par les écrans de saisie.
 * @details Responsabilités : Capturer le texte, proposer des suggestions en temps réel
 * et gérer les variantes de disposition (AZERTY/QWERTY, Symboles).
 * Dépendances principales : QDialog (fenêtre modale), widgets Qt, QSettings et timers d'interaction.
 */

#ifndef CLAVIER_H
#define CLAVIER_H

#include <QDialog>
#include <QPushButton>
#include <QGridLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QSettings>
#include <QTimer>
#include <QMap>

/**
 * @class Clavier
 * @brief Interface de saisie tactile sur mesure pour l'application embarquée.
 * * S'ouvre sous forme de boîte de dialogue modale (QDialog) bloquant le reste de l'UI.
 * * Implémente des mécaniques avancées : Majuscule automatique, verrouillage Maj (Caps Lock),
 * popups dynamiques pour les accents (appui long), et suppression rapide (maintien de la touche effacer).
 */
class Clavier : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur du clavier virtuel.
     * @param parent Widget parent.
     */
    explicit Clavier(QWidget *parent = nullptr);

    /**
     * @brief Récupère le texte final saisi par l'utilisateur une fois le clavier validé.
     * @return La chaîne de caractères courante.
     */
    QString getText() const;

    /**
     * @brief Pré-remplit la barre de saisie à l'ouverture du clavier.
     * @param text Le texte initial à afficher et modifier.
     */
    void setInitialText(const QString &text);

    /**
     * @brief Met à jour la liste déroulante des suggestions au-dessus du clavier.
     * @param suggestions Liste des mots ou adresses suggérés.
     */
    void displaySuggestions(const QStringList &suggestions);

signals:
    /**
     * @brief Émis à chaque fois qu'une touche modifie le texte.
     * Permet à l'application en arrière-plan (ex: NavigationPage) de lancer
     * des recherches d'autocomplétion en direct.
     * @param text Le contenu actuel de la barre de recherche.
     */
    void textChangedExternally(const QString &text);

private slots:
    // --- GESTION DES TOUCHES CLASSIQUES ---
    void handleButton();        ///< Traite l'appui sur une touche de lettre ou symbole standard.
    void deleteChar();          ///< Supprime le dernier caractère saisi.
    void validateText();        ///< Valide la saisie, ferme le clavier et sauvegarde l'historique.
    void toggleShift();         ///< Alterne entre Minuscule, Majuscule (1 lettre) et Caps Lock.
    void addSpace();            ///< Ajoute un espace.
    void addUnderscore();       ///< Ajoute un tiret du bas (_).

    // --- GESTION DES APPUI LONGS (ACCENTS) ---
    void handleLongPress();     ///< Déclenche l'affichage du menu des accents après un délai.
    void insertAccent();        ///< Insère l'accent sélectionné dans le menu popup.
    void startLongPress();      ///< Démarre le timer d'appui long lors de la pression d'une touche.
    void stopLongPress();       ///< Annule le timer d'appui long si la touche est relâchée tôt.

    // --- GESTION DE LA SUPPRESSION CONTINUE ---
    void startDeleteDelay();    ///< Démarre le timer initial avant d'enclencher la suppression rapide.
    void startDelete();         ///< Enclenche la suppression en boucle rapide.
    void stopDelete();          ///< Arrête la suppression en boucle.

    // --- GESTION DES DISPOSITIONS ---
    void switchKeyboard();      ///< Bascule entre le mode Lettres (ABC) et Chiffres/Symboles (123?).
    void switchLayout();        ///< Bascule entre les dispositions linguistiques (AZERTY / QWERTY).

private:
    // --- COMPOSANTS GRAPHIQUES ---
    QLineEdit *lineEdit;               ///< Champ d'affichage du texte saisi.
    QListWidget *suggestionList;       ///< Menu déroulant des suggestions en direct.
    QGridLayout *gridLayout;           ///< Grille de placement des touches du clavier.

    QPushButton *shiftButton;          ///< Touche Majuscule.
    QPushButton *switchButton;         ///< Touche de bascule (123? / ABC).
    QPushButton *langueButton;         ///< Touche de bascule AZERTY/QWERTY.
    QPushButton *apostropheButton;     ///< Touche raccourci apostrophe.
    QPushButton *currentLongPressButton = nullptr; ///< Garde en mémoire la touche en cours d'appui long.

    // --- LISTES DE TOUCHES (POUR BASCULE DYNAMIQUE) ---
    QList<QPushButton*> allButtons;    ///< Toutes les touches alphabétiques.
    QList<QPushButton*> symbolButtons; ///< Toutes les touches de symboles.
    QList<QPushButton*> row1Buttons;   ///< Touches de la 1ère rangée (A,Z,E...).
    QList<QPushButton*> row2Buttons;   ///< Touches de la 2ème rangée (Q,S,D...).
    QList<QPushButton*> row3Buttons;   ///< Touches de la 3ème rangée (W,X,C...).

    // --- GESTIONNAIRES D'INTERACTION ---
    QMap<QString, QStringList> accentMap; ///< Dictionnaire associant une lettre à ses variantes accentuées.
    QTimer *deleteTimer;                  ///< Timer de suppression rapide répétée (ex: 75ms).
    QTimer *deleteDelayTimer;             ///< Timer d'attente avant suppression rapide (ex: 500ms).
    QTimer *longPressTimer;               ///< Timer déterminant un appui long (ex: 500ms).
    QWidget *accentPopup = nullptr;       ///< Fenêtre volante affichant les choix d'accents.

    // --- ÉTAT INTERNE ---
    enum KeyboardLayout { AZERTY, QWERTY };
    KeyboardLayout currentLayout;         ///< Disposition linguistique actuelle.

    bool majusculeActive;                 ///< Vrai si la prochaine lettre doit être en majuscule.
    bool isSymbolMode;                    ///< Vrai si le clavier affiche les symboles (@, #, etc.).
    bool shiftLock = false;               ///< Vrai si le Caps Lock (Verrouillage Maj) est activé.

    QStringList usageHistory;             ///< Historique local des saisies passées.

    // --- MÉTHODES INTERNES ---
    void updateKeys();                    ///< Met à jour l'affichage des touches (majuscule/minuscule).
    void updateKeyboardLayout();          ///< Réorganise les lettres selon le layout choisi (AZERTY/QWERTY).
    void showAccentPopup(QPushButton* button); ///< Construit et affiche le popup des accents au-dessus de la touche.
    void hideAccentPopup();               ///< Masque le popup des accents s'il est ouvert.
    void loadUsageHistory();              ///< Charge l'historique de saisie depuis les QSettings.
    void saveUsageHistory() const;        ///< Sauvegarde l'historique de saisie dans les QSettings.
};

#endif // CLAVIER_H
