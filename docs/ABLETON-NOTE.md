# ChoralRoot × Ableton Note

Cette version de ChoralRoot (0.28-note1, basée sur 0.28) ajoute six réglages pour enregistrer le FM-1 dans
**Ableton Note** sur iPhone ou iPad : une part à la fois, sur les temps, dans la même tonalité. Tout le reste du
firmware est celui de 0.28 ; réglages à leur valeur par défaut, le FM-1 se comporte exactement comme avant.

## Ce qu'il faut savoir sur Note

D'après la documentation d'Ableton, Note reçoit les notes MIDI (hauteur, durée, vélocité, sustain) et l'aftertouch
polyphonique, dans la piste ouverte. On peut assigner au MIDI ses commandes Capture, Effacer la capture,
Start/Stop, Quantize, Undo et Redo, et 8 macros. Depuis Note 2, ses pistes audio enregistrent une entrée externe
(une interface USB, donc le FM-1).

Sa documentation ne mentionne ni sortie MIDI, ni horloge MIDI, ni tonalité ou tempo reçus par MIDI : sa synchro est
Ableton Link. D'où les choix ci-dessous : c'est le FM-1 qui démarre Note, et c'est toi qui règles la tonalité et le tempo
dans Note, une fois (le FM-1 t'affiche quoi y mettre).

## Les six réglages (Options, après Output)

| Réglage | Valeurs | Défaut | Ce que ça fait |
| --- | --- | --- | --- |
| **Part to Note** | All, Chords, Bass, Raw Chord | All | Quelle part sort en MIDI. Note met tout ce qui arrive dans la piste ouverte : avec Chords, seuls les accords partent ; avec Bass, seule la basse. Le FM-1, lui, continue de jouer les deux. **Raccourci : GLO maintenu + BASS** fait défiler All → Chords → Bass sans ouvrir les Options (la basse n'est pas coupée). |
| **Note Audio** | Master, Follow Part | Master | Ce qui passe sur les canaux 1-2 de l'entrée USB « ChoralRoot In » (ceux que Note enregistre). Follow Part : la part choisie dans Part to Note, **sèche** (avant reverb, chorus, delay), à -6 dB. Avec All ou Raw Chord : le master. |
| **Note Oct Chords** | -2 à +2 octaves | 0 | Décale les notes MIDI des accords (et de l'accord brut) envoyées à Note, pour coller au registre de l'instrument choisi dans Note. Le son du FM-1 ne change pas. |
| **Note Oct Bass** | -2 à +2 octaves | 0 | La même chose pour la basse. |
| **Note Start/Stop** | Off, ch 1 à 16 | Off | Quand la boucle du FM-1 démarre ou s'arrête (LOOP), le FM-1 envoie le CC 102 (127 puis 0, comme un bouton) sur ce canal. Assigné au Start/Stop de Note, ça lance et arrête Note **au même instant** que le FM-1. **OCT+ sur ce réglage envoie le CC tout de suite**, pour faire l'assignation dans Note. Pas envoyé par LOOP quand MIDI Clock est sur In. |
| **Note Setup** | OCT+ | | Règle tout d'un coup : USB Record On, USB Rate 44.1 + 48 kHz (iOS travaille à 48 kHz), USB Level Fixed, les canaux MIDI des accords et de la basse allumés s'ils étaient éteints, Part to Note sur Chords (s'il était sur All), Note Audio sur Follow Part, Note Start/Stop sur ch 16 (s'il était Off). **La ligne sous ce réglage affiche quoi mettre dans Note** : par exemple `Note: D Minor · 96 BPM`. |

Ces réglages sont enregistrés tout seuls, comme les autres Options, et Reset Settings les remet à leur défaut.

## Mise en route (une seule fois)

1. Branche le FM-1 sur l'iPhone ou l'iPad en USB (USB-C direct, ou adaptateur Lightning vers USB 3 pour appareil
   photo). Le FM-1 est « class compliant » : pas de pilote.
2. Sur le FM-1 : **GLO** (Options) → SELECT jusqu'à **Note Setup** → **OCT+**. Le FM-1 se reconnecte une seconde
   (`USB RECONNECTING`) : c'est normal.
3. Dans Note, **App Settings > Link & MIDI** : active le FM-1 comme source MIDI.
4. Dans Note, l'assignation MIDI : choisis l'assignation, touche **Start/Stop**, puis sur le FM-1 va sur
   **Options > Note Start/Stop** et appuie sur **OCT+** (le FM-1 affiche `CC 102 sent`). Valide dans Note.
5. Dans le Set de Note : **Set Settings > Key & Scale** = ce qu'affiche la ligne de Note Setup (le Key Mode du FM-1,
   avec son Transpose), et le **tempo** = le BPM du FM-1 (bouton SELECT du FM-1, ou l'inverse : règle le FM-1 sur le
   tempo de Note).

## Enregistrer une part en MIDI (les instruments de Note)

1. Ouvre la piste de Note qui doit recevoir la part (elle s'arme toute seule).
2. Sur le FM-1 : **GLO + BASS** jusqu'à `to Note: chords` (ou `bass`).
3. Joue. Dans Note : Capture (ou enregistrement sur la grille). Le MIDI se quantise dans Note après coup.
4. Passe à la part suivante : ouvre l'autre piste, **GLO + BASS** → `to Note: bass`, rejoue.

Note Oct Chords / Note Oct Bass servent quand l'instrument de Note sonne trop haut ou trop bas.

## Enregistrer le son du FM-1 (pistes audio de Note 2)

1. Dans Note, une piste audio avec comme entrée le FM-1 (« ChoralRoot In »).
2. Sur le FM-1, Note Audio sur **Follow Part** (Note Setup le fait) : les canaux 1-2 portent la part choisie, sèche.
   Pour le son complet avec les effets, mets Note Audio sur Master (et coupe la basse si tu ne veux que les accords).
3. Enregistre d'abord la progression dans le **looper** du FM-1 (REC, avec Quantize dans le menu PLAY) : la boucle
   est alors parfaitement sur la grille du FM-1.
4. Arme l'enregistrement de la piste dans Note, puis **LOOP** sur le FM-1 : la boucle et Note démarrent ensemble.
   Note « warpe » ses boucles audio : elles restent calées.
5. **GLO + BASS** pour passer à la basse, nouvelle piste audio, LOOP de nouveau.

## Rester sur les temps

- **Même tempo des deux côtés**, puis le départ commun par LOOP (Note Start/Stop). Les deux horloges sont des quartz :
  sur quelques minutes l'écart reste de l'ordre de la milliseconde.
- Si Note lance un décompte, coupe-le ou démarre depuis le FM-1 avec le Count-in du looper.
- La latence USB et audio de l'iPhone décale le départ de quelques millisecondes : les marqueurs de warp de Note
  (2.1) permettent de recaler une prise si besoin.
- **Synchro béton** : une app « pont » Ableton Link → horloge MIDI sur l'iPhone/iPad (il en existe plusieurs), avec
  le FM-1 sur **MIDI Clock : In**. Le FM-1 suit alors le tempo de Note et sa boucle suit Start/Stop ; Note
  Start/Stop n'envoie plus rien par LOOP (sinon il arrêterait ce qui l'a lancé).

## Rester dans la même tonalité

Allume le **Key Mode** du FM-1 (SEL) : les accords restent dans la gamme. La ligne de **Note Setup** donne la
tonalité exacte à mettre dans Note, Transpose compris (`Note: F Minor · 96 BPM`). Le Key Mode du FM-1 ne connaît
que Majeur et Mineur : dans Note, choisis Major ou Minor (les modes de Note n'ont pas d'équivalent dans le FM-1).
« In Key » dans Note ne change que ses pads, pas le MIDI reçu.

## L'expression

Options > **Key Touch : Vel + Poly AT** envoie l'aftertouch polyphonique, que Note enregistre sur ses presets qui
l'utilisent. Avec Note Oct, l'aftertouch suit les notes décalées.

## Limites

- Les clips de Note ne peuvent pas faire jouer le FM-1 : Note n'a pas de sortie MIDI.
- Note ne reçoit ni la tonalité ni le tempo : ils se règlent à la main, une fois par Set.
- La paire sèche (Follow Part) est avant les effets et à -6 dB : c'est voulu, pour garder de la marge.
- Cette version n'est pas une version officielle de ChoralRoot. Le code a passé les tests de l'ordinateur (réglages,
  MIDI, écran, mise en mémoire), pas encore un FM-1 réel. Fais une sauvegarde avant d'installer (l'installeur la propose).

## Installer, revenir en arrière

Le paquet `choralroot-0.28-note1.fwsc` s'installe avec l'outil en ligne de commande du projet
(`tools/fm1_install.py`, README « From the command line ») : il sauvegarde d'abord le FM-1, puis installe. Le web
installer officiel ne propose que ses propres versions ; pour revenir à une version officielle de ChoralRoot,
il suffit de l'ouvrir et d'appuyer sur Install : les sons, boucles et réglages sont gardés. Les réglages Note
disparaissent, et si l'un d'eux était activé, le réglage qui partage son octet reprend son défaut sur la version
officielle : MIDI Clock (Out), USB Level (Master), Raw Chord Sound et MIDI Keys (Off), ou l'activation MIDI d'une part.

## Pour qui lit le code

- Le stockage : `firmware/src/cr_settings.h`, « The Ableton Note Options » (des bits libres de `midi_en[]`,
  `raw_sound`, `usb_level` et `clock_mode`, sans changer de version d'enregistrement).
- La sortie : `firmware/src/cr_out.c` (`cr_route.out_mask`, `oct[]`, `remote`, `cr_osent`, `cr_rt_poll`,
  `cr_usb_pair1`), l'interface : `firmware/src/cr_ui.c` (`cu_note_route`, `cu_note_setup`, `cu_note_part_next`).
- Les tests : `tests/cr_note_test.c`, `tests/cr_note_ui_test.c`, `tests/cr_settings_test.c` (`t_note`).
- La compilation : `.github/workflows/build-note.yml`.
