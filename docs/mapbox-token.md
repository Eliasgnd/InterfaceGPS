# Configurer un token Mapbox (Raspberry Pi)

Ce guide explique comment créer un compte Mapbox, récupérer un token depuis le dashboard, puis le configurer sur Raspberry Pi pour permettre le calcul d’itinéraire dans InterfaceGPS.

## 1) Créer un compte Mapbox

1. Ouvrir [https://www.mapbox.com/](https://www.mapbox.com/).
2. Cliquer sur **Sign up** (ou **Get started for free**).
3. Créer le compte (email + mot de passe) puis valider l’email si demandé.

## 2) Récupérer un token dans le dashboard

1. Se connecter à [https://account.mapbox.com/](https://account.mapbox.com/).
2. Aller sur la section **Access tokens**.
3. Utiliser le token public par défaut (**Default public token**) ou créer un nouveau token avec **Create a token**.
4. Copier la valeur du token (elle commence en général par `pk.`).

> Pour InterfaceGPS, un token **public** suffit dans la plupart des cas.

## 3) Configurer le token sur Raspberry Pi

InterfaceGPS lit la variable d’environnement `MAPBOX_API_KEY`.

### Configuration temporaire (session en cours)

```bash
export MAPBOX_API_KEY="pk.votre_token_mapbox"
./InterfaceGPS
```

### Configuration persistante (recommandée)

Ajouter la variable dans `~/.bashrc` :

```bash
echo 'export MAPBOX_API_KEY="pk.votre_token_mapbox"' >> ~/.bashrc
source ~/.bashrc
```

Puis vérifier :

```bash
echo "$MAPBOX_API_KEY"
```

## 4) Vérifier que le calcul de trajet fonctionne

1. Lancer l’application depuis le même terminal (`./InterfaceGPS`).
2. Ouvrir l’écran de navigation.
3. Saisir une destination puis lancer une recherche d’itinéraire.
4. Vérifier qu’un trajet est bien calculé (distance/temps et tracé d’itinéraire disponibles).

Si aucun trajet ne se calcule :

- vérifier que la variable est bien définie (`echo "$MAPBOX_API_KEY"`),
- vérifier la connexion réseau,
- vérifier que le token n’a pas été révoqué dans le dashboard Mapbox,
- vérifier que le token dispose des droits nécessaires côté Mapbox (API Directions/Navigation selon la configuration du compte).

