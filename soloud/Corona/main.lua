--- Entry point.
--
-- The text below is adapted either from megademo code or http://solhsa.com/soloud/examples.html.

--
-- Permission is hereby granted, free of charge, to any person obtaining
-- a copy of this software and associated documentation files (the
-- "Software"), to deal in the Software without restriction, including
-- without limitation the rights to use, copy, modify, merge, publish,
-- distribute, sublicense, and/or sell copies of the Software, and to
-- permit persons to whom the Software is furnished to do so, subject to
-- the following conditions:
--
-- The above copyright notice and this permission notice shall be
-- included in all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
-- EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
-- IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
-- CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
-- TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
-- SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--
-- [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
--

-- Standard library imports --
local require = require

-- Modules --
local utils = require("utils")

-- Solar2D globals --
local native = native

--
--
--

local Menu

local function Choose (button)
  Menu:removeSelf()

  require("tests." .. button:getLabel())
end

--
--
--

utils.Begin("Select sub-demo to run", 50, 50)
utils.Begin("scrollview", 500, "hide")

local seps, any = {}

for _, v in ipairs{
{
    to = "quickstart",
    about = [[
Tiny demo from the homepage, that simply plays a panned sound.]]

  },
  {
    to = "simplest",
    about = [[
The simplest example initializes SoLoud, and uses the speech synthesizer
to play some sound. Once the sound has finished, the application cleans up and quits.]]

  },

  {
    to = "welcome",
    about = [[
Slightly more complicated console-based example, playing different kinds of sounds:

  Set up SoLoud
  Load and play looping ogg stream
  Adjust live parameters of the ogg (volume, pan, play speed)
  Ask for text input, play it through speech synthesizer
  Wait until speech is over
  Try to load and play an S3M module]]

  },

  {
    to = "enumerate",
    about = [[
The enumerate demo scans all included back-ends and shows data about them. Note that
some backends, like WinMM, only support limited number of channels, while others may
report more channels available than the hardware supports, like PortAudio.]]

  },

  {
    to = "env",
    about = [[
The env demo is a non-interactive demo of how SoLoud could be used to play environmental
audio. It is more of a proof of concept than a good example at this point.]]

  },
  
  {
    to = "null",
    about = [[
The null demo shows an example of using the null driver backend. It plays some sound and
draws the waveform on the console using ascii graphics.]]

  },

  {
    to = "c_test",
    about = [[
The c_test demo uses the "c" API to play voice samples as well as playing a wave that is
generated on the fly.]]

  },

  {
    to = "piano",
    about = [[
This example is a simple implementation of a playable instrument. The example also includes
a simple waveform generator, which can produce square, saw, sine and triangle waves.

You can jam from the PC keyboard (or with the piano keys widget).

You can also adjust some filters and pick waveforms using the GUI. Speech synthesizer
describes your option changes.]]

  },

  {
    to = "multimusic",
    about = [[
Multimusic demo plays multiple music tracks with interactive options to fade between them.]]

  },

  {
    to = "monotone",
    about = [[
Monotone demo plays a "monotone" tracker song with various interative options and filters.]]

  },

  {
    to = "tests.tedsid",
    about = [[
tedsid demonstrates the MOS TED and SID synthesis engines.]],
    NYI = true -- files missing / not loading

  },

  {
    to = "wavformats",
    about = [[
wavformats test plays files with all sorts of wave file formats.]]

  },

  {
    to = "speakers",
    about = [[
speakers test plays single sounds through surround speakers.]]

  },

  {
    to = "ay",
    about = [[
ay demonstrates the AY-3-8912 synthesis engine (zx spectrum 128k music).]],
    NYI = true -- files missing

  },

  {
    to = "mixbusses",
    about = [[
Mixbusses creates three different mix busses, plays different sounds in
each and lets user adjust the volume of the bus interactively.]]

  },

  {
    to = "test3d",
    about = [[
3d test demo is a test of the 3d sound.]]

  },

  {
    to = "pewpew",
    about = [[
pewpew demo shows the use of play_clocked and how delaying sound makes it
feel like it plays earlier.]]

  },

  {
    to = "radiogaga",
    about = [[
radiogaga demo demonstrates audio queues to generate endless music by
chaining random clips of music.]]

  },
  
  {
    to = "space",
    about = [[
space demo is a mockup of a conversation in a space game, and shows use
of the visualization interfaces.]]

  },

  {
    to = "speechfilter",
    about = [[
speechfilter demonstrates various DSP effects on speech synthesis.]]

  },
  
  {
    to = "virtualvoices",
    about = [[
virtualvoices demonstrates playing way more sounds than is actually possible,
and having the ones active that matter.]]

  },

  {
    to = "thebutton",
    about = [[
thebutton test shows one way of avoiding actor speaking on top of themselves.]]

  },

  {
    to = "annex",
    about = [[
annex test moves a live sound from one mixing bus to another.]]

  },

  {
    to = "filterfolio",
    about = [[
Filter folio is a playground for various filters and their parameters.]]

  },

  {
    to = "henley",
    about = [[
Henley spouts off random words based on patterns found in a piece of text.]]
  }
} do
  if not v.NYI then
    if any then
      seps[#seps + 1] = utils.Separator(25)
    else
      any = true
    end

    utils.Button{
      label = v.to, font = native.systemFontBold, font_size = 30,
      width = 275, height = 70,
      action = Choose
    }
    utils.NewLine(5)
    utils.Text(v.about)
  end
end

for i = 1, #seps do
  seps[i].width = utils.GetColumnWidth()
end

utils.End()

Menu = utils.End()