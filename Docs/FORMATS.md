# Plugweaver data formats v1

Both formats are ordinary ZIP archives. Entries are file paths, never executable scripts.
`document.json` is UTF-8 JSON. Paths are relative; no symlinks, absolute paths, parent
traversal or duplicate entries. Total uncompressed size <=512 MiB, <=400 files.
Only referenced assets are accepted. These formats are documented openly; no licence fee.

## Project: .artiplug

`format: "artilody.plugweaver"`, `version: 1`, `name`, `maker`, `uuid`, `theme`,
`defaults` (gain, attack, decay, sustain, release, cutoff, resonance, reverb, enabled,
filtertype 0-2 = low/band/high pass), `zones` (array of asset, low, high, root, velLow,
velHigh, optional `file` = original sample file name for display). MIDI fields are integers.
Audio is under `samples/`; graphics under `images/`. Audio is embedded, so moving
or deleting the original SFZ folder does not break a saved project.
The UUID gives the generated plugin a stable identity. New instrument => New project.

## Template: .artitheme

`format: "artilody.theme"`, `version: 1`, `theme`. Only used image assets are included.
No zones, parameter values, instrument identity or audio. Import registers it in the
local template gallery. Apply to any instrument; the keyboard coverage is recalculated.

## Theme object

`name`, `author`, `background`, `logo`, `fit` (cover, contain, stretch),
`area` (header = artwork in the top area with a wave-shaped lower edge, full = whole
1000x490 design area; missing in older files = full), `knobStyle` (classic, minimal),
`base`, `accent`, `ink` (eight-digit ARGB hex), `controls` array.
Controls: `type` (knob, slider, toggle, label, logo), `binding` (parameter ID), `text`,
`image` (optional PNG/JPEG for a control), `x`, `y`, `w`, `h`, `font`, `fontSize`,
`align` (left, centre, right), `colour` (optional ARGB hex; empty = theme text colour),
`spacing` (letter spacing 0-1), `style` (optional; knob: classic, minimal, vintage,
ledring, metal, flat; slider: classic, fader, bar, minimal, led, vertical; toggle: pill,
rocker, button, lever, led, slide; empty = theme `knobStyle` for knobs, classic/pill
otherwise). Theme `knobStyle` accepts every knob style. Text supports `{instrument}` and `{maker}`.
Font, fontSize, align and spacing apply to labels. A `logo` element shows the theme
logo at its position and size; themes without one show the logo top right.
Logical canvas is 1000x600, with controls confined to [0,0,1000,490]. Minimum control
size 30x24. Maximum 64 controls. The bottom 110 pixels always contain the live keyboard.
Knob graphic is a single image rotated around its centre; not a filmstrip.
Switches bind to enabled in v1. Parameter IDs/ranges are defined in EngineTemplate/Source/Model.h.

## SFZ import

Supports Sampleforge exports and basic SFZ with global/group/region/control headers,
sample, key, lokey, hikey, pitch_keycenter, lovel, hivel, loop_mode=no_loop and default_path.
WAV/AIFF/FLAC decoding uses JUCE. Numeric MIDI keys recommended. Text C4 means MIDI 60.
Unsupported opcodes, includes, macros and loop modes cause an explicit error rather
than silently changing the instrument. Maximum 256 regions, 256 MiB decoded PCM,
mono/stereo, 8–192 kHz. Relative paths must remain under the SFZ directory, without symlinks.
The .zip containing SFZ+WAV must be extracted before import.

## Generated plugins

`instrument.zip` is the project payload. It is embedded into the generated plugin binary,
along with image assets. Plugin editor and builder preview share the same C++ classes.
Source export includes all runtime sources, CMake configuration, licences and a Mac build
script, plus a Windows/Mac GitHub Actions workflow. JUCE and CLAP are fetched at build time.
Host parameter state is serialized separately with JUCE APVTS; the immutable instrument
payload remains embedded in the binary.
