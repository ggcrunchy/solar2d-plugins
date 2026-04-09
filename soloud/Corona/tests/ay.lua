--- Port of "megademo/ay".

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

-- Modules --
local utils = require("utils")

-- Plugins --
local soloud = require("plugin.soloud")

--
--
--

local gSoloud = soloud.createCore{ flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" } }
local gMusic = soloud.createAy()
local gBiquad = soloud.createBiquadResonantFilter()
local gEcho = soloud.createEchoFilter()
local gDCRemoval = soloud.createDCRemovalFilter()
local gBassboost = soloud.createBassboostFilter()

--
--
--

gMusic:load("audio/adversary.pt3_2ay.zak")

gEcho:setParams{ delay = 0.2, decay = 0.5, filter = 0.05 }
gBiquad:setParams{ type = "LOWPASS", frequency = 4000, resonance = 2 }

gMusic:setLooping(true)

gSoloud:setGlobalFilter(1, gBiquad)
gSoloud:setGlobalFilter(2, gBassboost)
gSoloud:setGlobalFilter(3, gEcho)
gSoloud:setGlobalFilter(4, gDCRemoval)

local gMusichandle = gSoloud:play(gMusic)

--
--
--

--[[
	float filter_param0[4] = { 0, 0, 0, 0 };
	float filter_param1[4] = { 1000, 2, 0, 0 };
	float filter_param2[4] = { 2, 0,  0, 0 };

	void DemoMainloop()
	{
		gSoloud.setFilterParameter(0, 0, 0, filter_param0[0]);
		gSoloud.setFilterParameter(0, 0, 2, filter_param1[0]);
		gSoloud.setFilterParameter(0, 0, 3, filter_param2[0]);

		gSoloud.setFilterParameter(0, 1, 0, filter_param0[1]);
		gSoloud.setFilterParameter(0, 1, 1, filter_param1[1]);

		gSoloud.setFilterParameter(0, 2, 0, filter_param0[2]);

		gSoloud.setFilterParameter(0, 3, 0, filter_param0[3]);

		float ayregs[32];
		int i;
		for (i = 0; i < 32; i++)
			ayregs[i] = gSoloud.getInfo(gMusichandle, i);
		ImGui::PlotHistogram("##AY", ayregs, 32, 0, "", 0, 0xff, ImVec2(264, 80), 4);

		ImGui::Text("AY1: %02X %02X %02X %02X %02X %02X %02X %02X", (int)ayregs[0], (int)ayregs[1], (int)ayregs[2], (int)ayregs[3], (int)ayregs[4], (int)ayregs[5], (int)ayregs[6], (int)ayregs[7]);
		ImGui::Text("     %02X %02X %02X %02X %02X %02X", (int)ayregs[8], (int)ayregs[9], (int)ayregs[10], (int)ayregs[11], (int)ayregs[12], (int)ayregs[13]);
		ImGui::Text("AY2: %02X %02X %02X %02X %02X %02X %02X %02X", (int)ayregs[16], (int)ayregs[17], (int)ayregs[18], (int)ayregs[19], (int)ayregs[20], (int)ayregs[21], (int)ayregs[22], (int)ayregs[23]);
		ImGui::Text("     %02X %02X %02X %02X %02X %02X", (int)ayregs[24], (int)ayregs[25], (int)ayregs[26], (int)ayregs[27], (int)ayregs[28], (int)ayregs[29]);

	}
}
]]

--
--
--

local histo = utils.MakeHistogram(.7, .3, .5)
local wave = utils.MakeWave(.3, .6, .8)

histo:translate(1500, 20)
wave:translate(1500, 240)

--
--
--

local AlignedText = utils.AlignedTextColumn(100, 100)
local Slider = utils.SliderColumn(300, 150, 500)

AlignedText("Biquad filter (lowpass)")
AlignedText(4)
Slider(
  "Wet",
  function(t)
		-- ImGui::SliderFloat("Wet##4", &filter_param0[0], 0, 1);
  end,
  0 --
)
Slider(
  "Frequency",
  function(t)
    -- ImGui::SliderFloat("Frequency##4", &filter_param1[0], 0, 8000);
  end
)
Slider(
  "Resonance",
  function(t)
    -- ImGui::SliderFloat("Resonance##4", &filter_param2[0], 1, 20);
  end
)
Slider(2)

AlignedText("Bassboost filter")
AlignedText(3)
Slider(
  "Wet",
  function(t)
    -- ImGui::SliderFloat("Wet##2", &filter_param0[1], 0, 1);
  end
)
Slider(
  "Boost",
  function(t)
    -- ImGui::SliderFloat("Boost##2", &filter_param1[1], 0, 11);
  end
)
Slider(2)

AlignedText("Echo filter")
AlignedText(2)
Slider(
  "Wet",
  function(t)
    -- ImGui::SliderFloat("Wet##3", &filter_param0[2], 0, 1);
  end
)
Slider(2)

AlignedText("DC removal filter")
Slider(
  "Wet",
  function(t)
    -- ImGui::SliderFloat("Wet##1", &filter_param0[3], 0, 1);
  end
)
--
--
--

timer.performWithDelay(50, function()
  utils.UpdateWave(wave, gSoloud:getWave())
	utils.UpdateHistogram(histo, gSoloud:calcFFT())
end, 0)