# Documentation du projet InterfaceGPS

Ce dossier regroupe la documentation complémentaire au code source.

## Contenu

- `assets/screenshots/` : captures d’écran de l’application (UI navigation, mode split, réglages, etc.).

## Documentation API

La documentation API C++ est générée avec Doxygen via le `Doxyfile` à la racine.

- Sortie locale attendue : `doc_output/html/index.html`
- Publication GitHub Pages : via workflow `.github/workflows/deploy_doc.yml`

## Bonnes pratiques

- Mettre à jour les captures lors des évolutions visuelles significatives.
- Conserver les commentaires Doxygen homogènes (`@file`, `@class`, `@brief`, `@param`, `@return`).
- Éviter les documents redondants avec le README principal.
