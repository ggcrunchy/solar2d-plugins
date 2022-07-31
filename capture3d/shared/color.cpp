#include "capture3d.h"

Color Color::operator * (float scale)
{
	Color out;

	for (int i = 0; i < 4; ++i) out.mBytes[i] = static_cast<unsigned char>(scale * mBytes[i]);

	return out;
}