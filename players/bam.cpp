/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999, 2000, 2001 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * bam.cpp - Bob's Adlib Music Player, by Simon Peter (dn.tlp@gmx.net)
 */

#include <string.h>
#include "bam.h"

static const unsigned short freq[] = {172,182,193,205,217,230,243,258,274,
290,307,326,345,365,387,410,435,460,489,517,547,580,614,651,1369,1389,1411,
1434,1459,1484,1513,1541,1571,1604,1638,1675,2393,2413,2435,2458,2483,2508,
2537,2565,2595,2628,2662,2699,3417,3437,3459,3482,3507,3532,3561,3589,3619,
3652,3686,3723,4441,4461,4483,4506,4531,4556,4585,4613,4643,4676,4710,4747,
5465,5485,5507,5530,5555,5580,5609,5637,5667,5700,5734,5771,6489,6509,6531,
6554,6579,6604,6633,6661,6691,6724,6758,6795,7513,7533,7555,7578,7603,7628,
7657,7685,7715,7748,7782,7819,7858,7898,7942,7988,8037,8089,8143,8191,8191,
8191,8191,8191,8191,8191,8191,8191,8191,8191,8191};

bool CbamPlayer::load(istream &f)
{
	char			id[4];

	f.seekg(0,ios::end); size = f.tellg() - 4; f.seekg(0);
	f.read(id,4);
	if(strncmp(id,"CBMF",4))
		return false;
	song = new unsigned char [size];
	f.read(song,size);
	rewind(0);
	return true;
}

bool CbamPlayer::update()
{
	unsigned char	cmd,c;

	if(del) {
		del--;
		return !songend;
	}

	if(pos >= size) {	// EOF detection
		pos = 0;
		songend = true;
	}

	while(song[pos] < 128) {
		cmd = song[pos] & 240;
		c = song[pos] & 15;
		switch(cmd) {
		case 0:		// stop song
			pos = 0;
			songend = true;
			break;
		case 16:	// start note
			if(c < 9) {
				opl->write(0xa0 + c, freq[song[++pos]] & 255);
				opl->write(0xb0 + c, (freq[song[pos]] >> 8) + 32);
			} else
				pos++;
			pos++;
			break;
		case 32:	// stop note
			if(c < 9)
				opl->write(0xb0 + c, 0);
			pos++;
			break;
		case 48:	// define instrument
			if(c < 9) {
				opl->write(0x20 + op_table[c],song[pos+1]);
				opl->write(0x23 + op_table[c],song[pos+2]);
				opl->write(0x40 + op_table[c],song[pos+3]);
				opl->write(0x43 + op_table[c],song[pos+4]);
				opl->write(0x60 + op_table[c],song[pos+5]);
				opl->write(0x63 + op_table[c],song[pos+6]);
				opl->write(0x80 + op_table[c],song[pos+7]);
				opl->write(0x83 + op_table[c],song[pos+8]);
				opl->write(0xe0 + op_table[c],song[pos+9]);
				opl->write(0xe3 + op_table[c],song[pos+10]);
				opl->write(0xc0 + c,song[pos+11]);
			}
			pos += 12;
			break;
		case 80:	// set label
			label[c].target = ++pos;
			label[c].defined = true;
			break;
		case 96:	// jump
			if(label[c].defined)
				switch(song[pos+1]) {
				case 254:	// infinite loop
					if(label[c].defined) {
						pos = label[c].target;
						songend = true;
						break;
					}
					// fall through...
				case 255:	// chorus
					if(!chorus && label[c].defined) {
						chorus = true;
						gosub = pos + 2;
						pos = label[c].target;
						break;
					}
					// fall through...
				case 0:		// end of loop
					pos += 2;
					break;
				default:	// finite loop
					song[pos+1]--;
					pos = label[c].target;
					break;
				}
			break;
		case 112:	// end of chorus
			if(chorus) {
				pos = gosub;
				chorus = false;
			} else
				pos++;
			break;
		default:	// reserved command (skip)
			pos++;
			break;
		}
	}
	if(song[pos] >= 128) {		// wait
		del = song[pos] - 127;
		pos++;
	}
	return !songend;
}

void CbamPlayer::rewind(unsigned int subsong)
{
	pos = 0; songend = false; del = 0; gosub = 0; chorus = false;
	memset(label,0,sizeof(label)); label[0].defined = true;
	opl->init(); opl->write(1,32);
}
