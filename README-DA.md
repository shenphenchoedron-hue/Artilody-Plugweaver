# Artilody Plugweaver — 0.2

C++20 / JUCE 7.0.12, GPLv3. Blå/orange instrumentbygger med SFZ-import, aflytning,
fri layoutdesigner, delbare templates og eksport/build af selvstændige instrumenter.
Ingen købt JUCE-licens, grafiklicens eller abonnement er nødvendigt for denne GPLv3-udgave.

## Byg på Mac

1. Pak ZIP-filen ud.
2. Skriv `bash` og et mellemrum i Terminal. Træk **Build-Mac.command** ind og tryk Enter.
3. Mangler Apple Command Line Tools, åbnes installationsdialogen. Kør filen igen bagefter.
4. Første build henter JUCE 7.0.12 og om nødvendigt CMake. Appen åbnes efter build og selvtest.

App: `build-mac/Plugweaver_artefacts/Release/Artilody Plugweaver.app`.
Minimum macOS 11. Scriptet bygger til den aktuelle Intel- eller Apple Silicon-Mac.
Behold projektets mappe, hvis du vil bruge dens lokalt hentede CMake til Build plugin.
Kopierer du kun appen, kan du stadig eksportere kildekode og bruge buildfilen dér.

## Byg på Windows

1. Installér [CMake](https://cmake.org/download/) (3.24 eller nyere) og Visual Studio 2022
   eller Build Tools med "Desktop development with C++".
2. Dobbeltklik **Build-Windows.bat**. Første build henter JUCE 7.0.12.

App: `build-windows\Plugweaver_artefacts\Release\Artilody Plugweaver.exe`.

## Byg på Linux

1. Ubuntu/Debian: `sudo apt install build-essential cmake git pkg-config libasound2-dev
   libjack-jackd2-dev libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev
   libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libgl1-mesa-dev`
2. Kør `bash Build-Linux.sh`.

App: `build-linux/Plugweaver_artefacts/Release/Artilody Plugweaver`.

GitHub Actions (`.github/workflows/build.yml`) bygger og tester appen på macOS, Windows og
Linux ved hvert push og bygger også et eksporteret testinstrument på alle tre platforme.
Færdige builds kan hentes under Actions > Artifacts.

## Første instrument

1. Eksportér **SFZ + WAV** fra Sampleforge. Pak eventuel ZIP ud først.
2. I Plugweaver: **Import SFZ**, og vælg `Instrument.sfz`.
3. Navngiv instrumentet og angiv Maker i højre side. Gem projektet som `.artiplug`.
4. Under **Sound** ser du samplelisten, bølgeformen, ADSR-kurven og filteret. Træk i
   punkterne på kurverne eller drej knapperne. Spil på det 88-tangenters keyboard nederst
   eller med MIDI-keyboard. Audio / MIDI (under Plugin settings) vælger lydudgang og MIDI.
   **Samples** viser alle regioner med grundtone, toneområde og velocity.
5. Under **Design** vælger du tema (Royal Amber, Violet Luxe, Graphite Glow), farver,
   baggrundsbillede og logo. Live preview viser det færdige plugin. Klik på et element
   (titel, maker-linje, logo, knapper) for at redigere det i "Selected element".
   Træk for at flytte; træk nederste højre hjørne for at ændre størrelse.
   Baggrunden kan ligge i toppen med bølgekant (Header wave) eller fylde hele designområdet.
   Logoet er et selvstændigt element, som kan flyttes og skaleres frit.
6. Tilføj Knob, Slider, Switch eller Text. Vælg parameter, tekstfarve, skrifttype, størrelse,
   bogstavafstand og justering.
7. Vælg udseende under **Controls**: alle varianter vises som forhåndsvisninger, og et
   klik bruger dem på det valgte element. "Use for all ..." giver alle af samme slags
   samme stil. Uden valgt element vælger du stilen for alle drejeknapper.
   - Drejeknapper: Classic, Minimal, Vintage, LED ring, Metal, Flat
   - Sliders: Classic, Fader, Bar, Minimal, LED meter, Vertical (lodret)
   - On/off: Pill, Rocker, Push button, Lever, LED key, Slide
8. Gem projektet. `.artiplug` indeholder både lyde, grafik, design og standardværdier.

Keyboardet viser altid 88 tangenter, A0–C8. Lyse tangenter med orange markering er
dækket af instrumentet; mørke tangenter er uden lyd. Huller udfyldes ikke automatisk.
MIDI-noter uden for det viste område kan stadig afspilles via MIDI, hvis SFZ'en dækker dem.

## Eget design og deling

Designområdet er 1000 × 490 pixels. Lav baggrunden i fx GIMP
eller Inkscape; importér PNG/JPEG. Du kan vælge udfyldning/beskæring, tilpasning eller
strækning. Farvefelter bruger seks hex-cifre; tryk Enter for at anvende dem.

Control image importerer egen grafik til den valgte drejeknap, slider eller switch.
En drejeknaps billede roterer; sliderens billede bruges som greb; en switch viser et
statusmærke. Der understøttes én billedfil pr. kontrol, ikke filmstrips eller separate
hover/pressed-billeder. Logoet er et element i designet: brug en PNG med transparent
baggrund, og placér/skalér det frit i Live preview.

Save template gemmer en `.artitheme` med layout, bindinger, farver og brugte billeder.
Lyde og lydindstillinger medtages ikke. Import template føjer delte templates til
menuen. Royal Amber er indbygget. Grafikken er data, og templates kan ikke køre kode.
Skabelonerne ligger i brugerens programdatamappe under Artilody/Plugweaver/Templates.

## Byg et plugin

Under Export vælges **VST3**, **AU** og/eller **CLAP**. AU bygges kun på Mac.

**Build plugin** beder om en overmappe, opretter en ny instrument-kildemappe og bygger
de valgte formater. Første build kræver internet. Det kan tage flere minutter.
Alle lyde og billeder indlejres i plugin'et; brugeren behøver ikke Plugweaver.
Der bygges også en standalone-version til afprøvning.

Outputs: `<instrumentmappe>/build/Instrument_artefacts/Release/`.
Ved fejl ligger `build.log` og kildekoden i den eksporterede mappe.
På Mac bygger **Build plugin** til både Intel og Apple Silicon, foretager lokal
ad-hoc-signering og installerer de valgte formater i brugerens pluginmapper:

- VST3: `~/Library/Audio/Plug-Ins/VST3/`
- AU: `~/Library/Audio/Plug-Ins/Components/`
- CLAP: `~/Library/Audio/Plug-Ins/CLAP/`

Hvert build får et nyt versionsnummer, gamle kopier fjernes før installation (ellers kan
gamle filer ødelægge signaturen), og macOS' AU-liste opdateres automatisk.
Luk musikprogrammet helt, åbn det igen, og scan plugins igen:

- Logic/GarageBand: Indstillinger > Plug-in Manager > Reset & Rescan Selection.
- REAPER: Settings > Plug-ins > VST/CLAP > Re-scan (Clear cache/re-scan ved problemer).
- FL Studio: Options > Manage plugins > Find installed plugins.

Find dem under instrumenter (producent = Maker), ikke audioeffekter. Logic bruger AU; Pro Tools' AAX-format understøttes ikke.
Build-Mac.command i nye eksporterede instrumentmapper bygger og installerer på samme måde.

På Windows og Linux bygger og installerer **Build plugin** (og Build-Windows.bat /
Build-Linux.sh i instrumentmappen) i brugerens egne pluginmapper:

- Windows: `%LOCALAPPDATA%\Programs\Common\VST3\` og `...\CLAP\`
- Linux: `~/.vst3/` og `~/.clap/`

På Windows kræver **Build plugin** også [Git](https://git-scm.com/download/win), som bruges
til at hente CLAP-delen. Scanner dit musikprogram ikke disse mapper, så tilføj dem i programmets pluginindstillinger.

**Export source project** opretter samme selvstændige kildeprojekt uden at bygge det.
Det indeholder `instrument.zip`, C++-koden, CMake, licenser, buildfiler til Mac, Windows og
Linux og et GitHub Actions-workflow til Windows x64, Linux og Mac Intel/Apple Silicon. Kilo Code kan arbejde
videre direkte i denne mappe. Plugweaver selv har også et sådant workflow.

Instrumentets identitet bevares i projektet ved geneksport. Brug **New** til et andet
instrument, så det får sin egen identitet. Et plugin ændrer ikke navn eller design,
fordi du efterfølgende ændrer projektet: det skal eksporteres og bygges igen.

## Baseline-01: bevidste afgrænsninger

- SFZ-parseren understøtter Sampleforge-eksport og en grundlæggende del af SFZ.
  Avancerede opcodes, scripts, includes, loops og macros afvises tydeligt.
- Maks. 256 regioner og 256 MiB dekodet mono/stereo-lyd. Der indlæses i RAM.
- 32 stemmers polyfoni, velocity, ADSR, lowpass/resonans, reverb, volumen,
  sustainpedal og pitchbend ±2 halvtoner. Transponering bruger interpolation og
  ændrer længden ved andre toner end samplets rod. Ingen time-stretch eller streaming.
- Designområdet er 1000 × 490; et 88-tangenters keyboard (A0–C8) ligger altid nederst i
  det 1000 × 600 store plugin.
  Hele plugin'et skalerer med vinduet. Maks. 64 kontroller. Ingen undo/redo i denne baseline.
- Egne scripts og vilkårlige DSP-funktioner kan ikke indlæses via templates.
- Plugin-standardværdier gemmes i projektet. DAW'en kan gemme/automatisere parametre.
- Ingen Apple-notarisering eller Windows-kodesignering er inkluderet.

## Test og kode

Mac-builds er testet lokalt (auval, VST3/AU/CLAP i testvært). Windows- og Linux-builds
testes i GitHub Actions; indlæsning i DAW'er på de platforme skal bekræftes på de maskiner. Det er en første baseline,
ikke en færdig gennemtestet udgivelse.

- Source/Main.cpp: builder og designer.
- Source/Ui.h: grafiske byggeklodser til builderen (logo, ikoner, paneler, grafer).
- Source/SfzImport.h: SFZ-parser.
- Source/Exporter.h: selvstændigt kildeprojekt.
- EngineTemplate/Source/: fælles model, lydmotor, plugin-editor og processor.
- Tests/: automatiske roundtrip-, lyd- og eksporttests.
- Docs/FORMATS.md: de åbne projekt- og templateformater.
- Docs/LICENSES-DA.md og LICENSE: GPLv3 og afhængigheder.
