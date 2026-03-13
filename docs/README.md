# Documentation du projet InterfaceGPS

Ce dossier regroupe la documentation complémentaire au code source.

## Contenu

- `mainpage.md` : page d’accueil Doxygen.
- `architecture.md` : vue d’ensemble architecture + flux principaux.
- `assets/screenshots/` : captures d’écran de l’application.
- `theme/` : thème Doxygen moderne (vendor + surcharges projet).

## Documentation API

La documentation API C++ est générée avec Doxygen via le `Doxyfile` à la racine.

- Déploiement GitHub Pages : `.github/workflows/doxygen-pages.yml` (push sur `main`)
- Sortie locale attendue : `doc_output/html/index.html`
- Génération locale : `bash scripts/run_doxygen.sh`

## Bonnes pratiques

- Mettre à jour les captures lors des évolutions visuelles significatives.
- Conserver les commentaires Doxygen homogènes (`@file`, `@class`, `@brief`, `@param`, `@return`).
- Éviter les documents redondants avec le README principal.
- Garder les styles Doxygen dans `docs/theme/` pour un workflow CI simple et maintenable.
