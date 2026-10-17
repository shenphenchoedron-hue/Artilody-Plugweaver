# Artilody Plugweaver — 0.3.1

C++20 / JUCE 7.0.12, GPLv3. Blå/orange instrumentbygger med SFZ-import, aflytning,
fri layoutdesigner, delbare templates og færdige VST3-, AU- og CLAP-instrumenter.
Ingen købt JUCE-licens, grafiklicens eller abonnement er nødvendigt for denne GPLv3-udgave.

## Installation

Hent installeren til dit system under
[Releases](https://github.com/shenphenchoedron-hue/Artilody-Plugweaver/releases):

| System | Installer |
|---|---|
| macOS 11+ (Intel og Apple Silicon) | `Artilody-Plugweaver-<version>-macOS.pkg` |
| Windows 10/11 x64 | `Artilody-Plugweaver-<version>-Windows-Setup.exe` |
| Ubuntu 22.04+ / Debian 12+ x64 | `artilody-plugweaver_<version>_amd64.deb` |

Alt er indbygget: appen indeholder en færdigbygget plugin-motor, så **Build plugin**
laver og installerer plugins på få sekunder uden compiler, CMake, Git eller internet.
Hverken appen eller de færdige plugins kræver ekstra filer eller runtime-pakker.

- **Mac:** Installeren er ikke notariseret af Apple. Højreklik på `.pkg` > **Åbn** > **Åbn**
  (eller Systemindstillinger > Anonymitet og sikkerhed > **Åbn alligevel**). Appen lægges i
  Programmer.
- **Windows:** SmartScreen kan advare, fordi installeren ikke er kodesigneret. Vælg
  **Flere oplysninger** > **Kør alligevel**. Installeren installerer som standard kun for dig
  (uden administratorrettigheder); du kan vælge alle brugere.
- **Linux:** `sudo apt install ./artilody-plugweaver_<version>_amd64.deb`. Start fra
  programmenuen eller med `artilody-plugweaver`.

## Byg selv fra kildekoden (udviklere)

Kræver en C++-compiler, CMake 3.24+ og Git. Første build henter JUCE 7.0.12 og
clap-juce-extensions og bygger både appen og plugin-motoren, som indlejres i appen.

- **Mac:** `bash Build-Mac.command` (Apple Command Line Tools; bygger til Intel og Apple
  Silicon). App: `build-mac/Plugweaver_artefacts/Release/Artilody Plugweaver.app`.
- **Windows:** Visual Studio 2022 / Build Tools ("Desktop development with C++"), CMake og
  Git; dobbeltklik **Build-Windows.bat**. App: `build-windows\Plugweaver_artefacts\Release\`.
- **Linux:** `sudo apt install build-essential cmake git pkg-config libasound2-dev
  libjack-jackd2-dev libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev
  libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libgl1-mesa-dev`,
  derefter `bash Build-Linux.sh`.

Installerne laves med `packaging/mac/build-pkg.sh`, `packaging/windows/Plugweaver.iss`
(Inno Setup 6) og `packaging/linux/build-deb.sh`. GitHub Actions bygger, tester og pakker
alt på macOS, Windows og Linux ved hvert push, installerer installeren og lader den
installerede app bygge og installere plugins. Resultatet ligger under Actions > Artifacts.

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

Under Export vælges **VST3**, **AU** og/eller **CLAP**. AU findes kun på Mac.

**Build plugin** laver de valgte formater ud fra den indbyggede plugin-motor og
installerer dem med det samme – typisk på få sekunder og uden andre programmer.
Alle lyde, billeder og indstillinger ligger inde i plugin'et; brugeren behøver ikke
Plugweaver. Plugins installeres i computerens almindelige pluginmapper, som alle
musikprogrammer kigger i, og som kan ses i Finder/Stifinder:

| System | VST3 | AU | CLAP |
|---|---|---|---|
| macOS | `/Library/Audio/Plug-Ins/VST3/` | `/Library/Audio/Plug-Ins/Components/` | `/Library/Audio/Plug-Ins/CLAP/` |
| Windows | `C:\Program Files\Common Files\VST3\` | – | `C:\Program Files\Common Files\CLAP\` |
| Linux | `~/.vst3/` | – | `~/.clap/` |

Øverst i Export-fanen står **Install plugins in:** med den mappe, plugins lægges i. Med
**Choose folder…** vælger du selv mappen (valget huskes), og **Use standard** går tilbage til
standardmappen. Plugins lægges i mappens undermapper `VST3`, `Components` (AU) og `CLAP`,
og den fulde sti til hver fil står lige nedenunder. Vælger du en anden mappe end
standardmappen, skal den tilføjes i musikprogrammets pluginindstillinger.

På Mac er standardmappen **Macintosh HD → Bibliotek → Audio → Plug-Ins** (ikke den skjulte Bibliotek-mappe
i din brugermappe). Mapperne tilhører systemet, så macOS beder om din adgangskode, og
Windows viser sin normale tilladelsesdialog, når et plugin installeres. Ældre kopier af
samme instrument i de skjulte brugermapper (fra Plugweaver 0.3.0) fjernes automatisk,
så musikprogrammet ikke ser instrumentet to gange.

**Du kan altid se, at filerne er lavet:**

- Efter hvert build viser appen en besked med alle filer og deres placering, med knapperne
  **Show installed files** og **Open copy**.
- Export-fanen viser for hvert format den fulde sti, og om det er installeret (med
  tidspunkt). Knappen **Show** åbner Finder/Stifinder direkte ved filen.
- En synlig kopi af de færdige plugins gemmes i `Dokumenter/Artilody Plugweaver/Plugins/<instrument>/`
  sammen med en `Read me.txt`, der fortæller, hvor de er installeret. Kopien kan sendes
  videre og installeres på en anden computer; musikprogrammerne bruger de installerede filer.

På Mac er plugins universelle (Intel og Apple Silicon) og ad-hoc-signeret med macOS'
indbyggede `codesign`. Hvert build får et nyt versionsnummer, gamle kopier fjernes før
installation, og macOS' AU-liste opdateres automatisk. Luk musikprogrammet helt, åbn det
igen, og scan plugins igen:

- Logic/GarageBand: Indstillinger > Plug-in Manager > Reset & Rescan Selection.
- REAPER: Settings > Plug-ins > VST/CLAP > Re-scan (Clear cache/re-scan ved problemer).
- FL Studio: Options > Manage plugins > Find installed plugins.

Find dem under instrumenter (producent = Maker), ikke audioeffekter. Logic bruger AU;
Pro Tools' AAX-format understøttes ikke. På Windows skal musikprogrammet være lukket,
når et instrument geninstalleres.

**Export source project** (for udviklere) opretter et selvstændigt C++/CMake-projekt med
`instrument.zip`, licenser, buildfiler til Mac, Windows og Linux og et GitHub
Actions-workflow. Det kræver compiler, CMake og Git at bygge; det gør **Build plugin** ikke.
Begge veje giver samme plugin-identitet, så DAW-projekter virker med begge.

Instrumentets identitet bevares i projektet ved genbygning. Brug **New** til et andet
instrument, så det får sin egen identitet. Et plugin ændrer ikke navn eller design,
fordi du efterfølgende ændrer projektet: det skal bygges igen.

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

Den automatiske test bygger to instrumenter med plugin-motoren og indlæser dem side om side
i en VST3-vært (navn, producent, unikt id og lyd) på macOS, Windows og Linux. På Mac er
auval og VST3/AU/CLAP i testvært også kørt lokalt. Indlæsning i DAW'er på Windows og Linux
skal bekræftes på de maskiner.

- Source/Main.cpp: builder og designer.
- Source/Ui.h: grafiske byggeklodser til builderen (logo, ikoner, paneler, grafer).
- Source/SfzImport.h: SFZ-parser.
- Source/Exporter.h: plugins fra motoren, installation og kildeprojekt.
- Source/EngineBlob.cpp: adgang til den indlejrede plugin-motor.
- EngineTemplate/Source/: fælles model, lydmotor, plugin-editor og processor.
- EngineTemplate/Source/PwIdentity.*, PwPayload.h: motorens identitet og instrumentdata.
- packaging/: installere til Mac, Windows og Linux.
- Tests/: automatiske roundtrip-, lyd- og eksporttests.
- Docs/FORMATS.md: de åbne projekt- og templateformater.
- Docs/LICENSES-DA.md og LICENSE: GPLv3 og afhængigheder.
