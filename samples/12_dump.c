#include <x/dump.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>


int main(void)
{
	x_dump *dmp = x_dump_block(x_u("Locations"), 
			x_dump_pair(x_dump_symbol(x_u("Beijing")),
				x_dump_block(x_u("Geo"), x_dump_float(116.405289), x_dump_float(39.904987), NULL)),
			x_dump_pair(x_dump_symbol(x_u("Tianjin")),
				x_dump_block(x_u("Geo"), x_dump_float(117.190186), x_dump_float(39.125595), NULL)),
			x_dump_pair(x_dump_symbol(x_u("Suihua")),
				x_dump_block(x_u("Geo"), x_dump_float(126.992928), x_dump_float(46.637394), NULL)),
			x_dump_pair(x_dump_symbol(x_u("Dalian")),
				x_dump_block(x_u("Geo"), x_dump_float(121.618622), x_dump_float(38.914589), NULL)),
			x_dump_pair(x_dump_symbol(x_u("Xianyang")),
				x_dump_block(x_u("Geo"), x_dump_float(108.705116), x_dump_float(34.333439), NULL)),
			x_dump_pair(x_dump_symbol(x_u("Shanghai")),
				x_dump_block(x_u("Geo"), x_dump_float(121.472641), x_dump_float(31.231707), NULL)),
			NULL);

	x_dump_fput(dmp, x_dump_pretty_format(), stdout);
	return 0;
}
