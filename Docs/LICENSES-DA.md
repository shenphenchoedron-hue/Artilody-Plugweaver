# Licenser og afhængigheder

Plugweavers originale C++-kode og genererede instrumentkode udgives under GPL-3.0-only.
Se LICENSE. Vi bruger den gratis GPLv3-mulighed, ikke en kommerciel JUCE-licens.

- JUCE er fastlåst til 7.0.12 (GPLv3/ISC-moduler). Hashen på kildearkivet verificeres.
  JUCE 8 og senere har en anden open source-licens og er ikke en direkte GPLv3-erstatning.
- VST3-SDK'et, som følger med JUCE 7.0.12, anvendes under sin GPLv3-mulighed.
- clap-juce-extensions er MIT, fastlåst til commit
  7adee3a1bd4684d4caa5601100e364abccff4b4d. Dets CLAP- og clap-helpers-submoduler er MIT.
- CMake er BSD-3-Clause. Mac-scriptet kan hente en verificeret privat kopi af CMake 3.31.6.
- Grafikken tegnes med JUCE. Intet ekstra grafikframework, abonnement eller betalt SDK.
- Medfølgende originalt ikon og designguide kan genbruges under CC0-1.0.
  Systemskrifter indlejres ikke.
- JUCE medbringer bl.a. FLAC/Ogg/Vorbis, JPEG, PNG/zlib og platformsafhængig kode
  under deres egne kompatible licenser. Bevar deres notices i kildeudgivelser.

Når du deler GPLv3-binaries, skal du også opfylde GPLv3-kravene om tilsvarende
kildekode, herunder nødvendige afhængigheder, byggeopsætning og ændringer. De
medfølgende kildereferencer er en byggeopsætning, ikke i sig selv en fuldstændig
kildekodeleverance til en binærudgivelse. Kilo Code kan hjælpe med at pakke dette
korrekt til GitHub. Det samme gælder instrumenter, som Plugweaver genererer.

Samples, fotos og andet materiale, som brugerne selv importerer, beholder deres
oprindelige rettigheder. Brug indhold, der må distribueres på den ønskede måde.
Templates indeholder data og billeder, ikke eksekverbar kode.

Apple Command Line Tools bruges til lokale Mac-builds. Et betalt Apple Developer-
abonnement er ikke nødvendigt for lokal build og ad-hoc-signering. Offentlig
notarisering er ikke inkluderet. GitHub Actions er valgfrit; alle builds kan køres lokalt.
