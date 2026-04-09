--- Port of "megademo/tedsid".

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

--[[
namespace tedsid
{
	float filter_param0[4] = { 0, 0, 0, 0 };
	float filter_param1[4] = { 1000, 2, 0, 0 };
	float filter_param2[4] = { 2, 0,  0, 0 };

	float song1volume = 1;
	float song2volume = 0;

	void DemoMainloop()
	{
		gSoloud.setFilterParameter(0, 0, 0, filter_param0[0]);
		gSoloud.setFilterParameter(0, 0, 2, filter_param1[0]);
		gSoloud.setFilterParameter(0, 0, 3, filter_param2[0]);

		gSoloud.setFilterParameter(0, 1, 0, filter_param0[1]);
		gSoloud.setFilterParameter(0, 1, 1, filter_param1[1]);

		gSoloud.setFilterParameter(0, 2, 0, filter_param0[2]);

		gSoloud.setFilterParameter(0, 3, 0, filter_param0[3]);


		DemoUpdateStart();

		float *buf = gSoloud.getWave();
		float *fft = gSoloud.calcFFT();

		ONCE(ImGui::SetNextWindowPos(ImVec2(500, 20)));
		ImGui::Begin("Output");
		ImGui::PlotLines("##Wave", buf, 256, 0, "Wave", -1, 1, ImVec2(264, 80));
		ImGui::PlotHistogram("##FFT", fft, 256 / 2, 0, "FFT", 0, 10, ImVec2(264, 80), 8);
		float sidregs[32 + 5];
		int i;
		for (i = 0; i < 32; i++)
			sidregs[i] = gSoloud.getInfo(gMusichandle1, i);
		for (i = 0; i < 5; i++)
			sidregs[32 + i] = gSoloud.getInfo(gMusichandle2, i + 64);
		ImGui::PlotHistogram("##SID", sidregs, 32 + 5, 0, "          SID               TED", 0, 0xff, ImVec2(264, 80), 4);

		ImGui::Text("SID: %02X %02X %02X %02X %02X %02X %02X %02X", (int)gSoloud.getInfo(gMusichandle1, 0), (int)gSoloud.getInfo(gMusichandle1, 1), (int)gSoloud.getInfo(gMusichandle1, 2), (int)gSoloud.getInfo(gMusichandle1, 3), (int)gSoloud.getInfo(gMusichandle1, 4), (int)gSoloud.getInfo(gMusichandle1, 5), (int)gSoloud.getInfo(gMusichandle1, 6), (int)gSoloud.getInfo(gMusichandle1, 7));
		ImGui::Text("     %02X %02X %02X %02X %02X %02X %02X %02X", (int)gSoloud.getInfo(gMusichandle1, 8), (int)gSoloud.getInfo(gMusichandle1, 9), (int)gSoloud.getInfo(gMusichandle1, 10), (int)gSoloud.getInfo(gMusichandle1, 11), (int)gSoloud.getInfo(gMusichandle1, 12), (int)gSoloud.getInfo(gMusichandle1, 13), (int)gSoloud.getInfo(gMusichandle1, 14), (int)gSoloud.getInfo(gMusichandle1, 15));
		ImGui::Text("     %02X %02X %02X %02X %02X %02X %02X %02X", (int)gSoloud.getInfo(gMusichandle1, 16), (int)gSoloud.getInfo(gMusichandle1, 17), (int)gSoloud.getInfo(gMusichandle1, 18), (int)gSoloud.getInfo(gMusichandle1, 19), (int)gSoloud.getInfo(gMusichandle1, 20), (int)gSoloud.getInfo(gMusichandle1, 21), (int)gSoloud.getInfo(gMusichandle1, 22), (int)gSoloud.getInfo(gMusichandle1, 23));
		ImGui::Text("     %02X %02X %02X %02X %02X %02X %02X %02X", (int)gSoloud.getInfo(gMusichandle1, 24), (int)gSoloud.getInfo(gMusichandle1, 25), (int)gSoloud.getInfo(gMusichandle1, 26), (int)gSoloud.getInfo(gMusichandle1, 27), (int)gSoloud.getInfo(gMusichandle1, 28), (int)gSoloud.getInfo(gMusichandle1, 29), (int)gSoloud.getInfo(gMusichandle1, 30), (int)gSoloud.getInfo(gMusichandle1, 31));
		ImGui::Text("TED: %02X %02X %02X %02X %02X", (int)gSoloud.getInfo(gMusichandle2, 64), (int)gSoloud.getInfo(gMusichandle2, 65), (int)gSoloud.getInfo(gMusichandle2, 66), (int)gSoloud.getInfo(gMusichandle2, 67), (int)gSoloud.getInfo(gMusichandle2, 68));
		ImGui::End();

		ONCE(ImGui::SetNextWindowPos(ImVec2(20, 20)));
		ImGui::Begin("Control");
		ImGui::Text("Song volumes");

		if (ImGui::SliderFloat("Song1 vol", &song1volume, 0, 1))
		{
			gSoloud.setVolume(gMusichandle1, song1volume);
		}
		if (ImGui::SliderFloat("Song2 vol", &song2volume, 0, 1))
		{
			gSoloud.setVolume(gMusichandle2, song2volume);
		}
]]

-- Modules --
local utils = require("utils")

-- Plugins --
local soloud = require("plugin.soloud")

--
--
--

local gSoloud = soloud.createCore{ flags = { "CLIP_ROUNDOFF", "ENABLE_VISUALIZATION" } }
local gMusic1, gMusic2 = soloud.createTedSid(), soloud.createTedSid()
local gBiquad = soloud.createBiquadResonantFilter()
local gEcho = soloud.createEchoFilter()
local gDCRemoval = soloud.createDCRemovalFilter()
local gBassboost = soloud.createBassboostFilter()

--
--
--

print("1",gMusic1:load("audio/Modulation.sid.dump"))
print("2",gMusic2:load("audio/ted_storm.prg.dump"))

gEcho:setParams{ delay = 0.2, decay = 0.5, filter = 0.05 }
gBiquad:setParams{ type = "LOWPASS", frequency = 4000, resonance = 2 }

gMusic1:setLooping(true)
gMusic2:setLooping(true)

gSoloud:setGlobalFilter(1, gBiquad)
gSoloud:setGlobalFilter(2, gBassboost)
gSoloud:setGlobalFilter(3, gEcho)
gSoloud:setGlobalFilter(4, gDCRemoval)

local gMusichandle1 = gSoloud:play(gMusic1)
local gMusichandle2 = gSoloud:play(gMusic2, 0)

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

--[[
		ImGui::Separator();
		ImGui::Text("Biquad filter (lowpass)");
		ImGui::SliderFloat("Wet##4", &filter_param0[0], 0, 1);
		ImGui::SliderFloat("Frequency##4", &filter_param1[0], 0, 8000);
		ImGui::SliderFloat("Resonance##4", &filter_param2[0], 1, 20);
		ImGui::Separator();
		ImGui::Text("Bassboost filter");
		ImGui::SliderFloat("Wet##2", &filter_param0[1], 0, 1);
		ImGui::SliderFloat("Boost##2", &filter_param1[1], 0, 11);
		ImGui::Separator();
		ImGui::Text("Echo filter");
		ImGui::SliderFloat("Wet##3", &filter_param0[2], 0, 1);
		ImGui::Separator();
		ImGui::Text("DC removal filter");
		ImGui::SliderFloat("Wet##1", &filter_param0[3], 0, 1);
	}
-- TODO: ^^^ out of whack somewhere, Bassboost or Lofi?
}
]]

--
--
--

timer.performWithDelay(50, function()
  utils.UpdateWave(wave, gSoloud:getWave())
	utils.UpdateHistogram(histo, gSoloud:calcFFT())
end, 0)