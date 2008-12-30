/*
*****************************************************************************
* COPYRIGHT AND WARRANTY INFORMATION
*
* Copyright 2003, Advanced Audio Video Coding Standard, Part II
*
* DISCLAIMER OF WARRANTY
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS"
* basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
* License for the specific language governing rights and limitations under
* the License.
*                     
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE AVS PATENT POLICY.
* The AVS Working Group doesn't represent or warrant that the programs
* furnished here under are free of infringement of any third-party patents.
* Commercial implementations of AVS, including shareware, may be
* subject to royalty fees to patent holders. Information regarding
* the AVS patent policy for standardization procedure is available at 
* AVS Web site http://www.avs.org.cn. Patent Licensing is outside
* of AVS Working Group.
*
* THIS IS NOT A GRANT OF PATENT RIGHTS - SEE THE AVS PATENT POLICY.
************************************************************************
*/

/*
*************************************************************************************
* File name: rdopt_coding_state.c
* Function:  
    Storing/restoring coding state for
    Rate-Distortion optimized mode decision
*
*************************************************************************************
*/


#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include "global.h"

void c_avs_enc:: delete_coding_state (CSptr cs)
{
  if (cs != NULL)
  {
    if (cs->bitstream != NULL)
      free (cs->bitstream);

    //=== coding state structure ===
    free (cs);
    cs=NULL;
  }

}

CSptr c_avs_enc:: create_coding_state ()
{
  CSptr cs;

  //=== coding state structure ===
  if ((cs = (CSptr) calloc (1, sizeof(CSobj))) == NULL)
    no_mem_exit("init_coding_state: cs");

  //=== important variables of data partition array ===

  if ((cs->bitstream = (Bitstream*) calloc (1, sizeof(Bitstream))) == NULL)
    no_mem_exit("init_coding_state: cs->bitstream");

  return cs;
}

void c_avs_enc::store_coding_state (CSptr cs)
{
  Bitstream            *bs_src, *bs_dest;
  Macroblock           *currMB  = &(img->mb_data [img->current_mb_nr]);

  if (!input->rdopt)
    return;

  //=== important variables of data partition array ===
  bs_src  =  currBitStream;
  bs_dest = cs->bitstream;

  memcpy (bs_dest, bs_src, sizeof(Bitstream));

  //=== syntax element number and bitcounters ===
  cs->currSEnr = currMB->currSEnr;
  memcpy (cs->bitcounter, currMB->bitcounter, MAX_BITCOUNTER_MB*sizeof(int_32_t));

  //=== elements of current macroblock ===
  memcpy (cs->mvd, currMB->mvd, 2*2*BLOCK_MULTIPLE*BLOCK_MULTIPLE*sizeof(int_32_t));
  cs->cbp_bits = currMB->cbp_bits;
}

void c_avs_enc:: reset_coding_state (CSptr cs)
{
  Bitstream            *bs_src, *bs_dest;
  Macroblock           *currMB  = &(img->mb_data [img->current_mb_nr]);

  if (!input->rdopt)
    return;

  bs_dest =   currBitStream;
  bs_src  =   cs->bitstream;

  memcpy (bs_dest, bs_src, sizeof(Bitstream));

  //=== syntax element number and bitcounters ===
  currMB->currSEnr = cs->currSEnr;
  memcpy (currMB->bitcounter, cs->bitcounter, MAX_BITCOUNTER_MB*sizeof(int_32_t));

  //=== elements of current macroblock ===
  memcpy (currMB->mvd, cs->mvd, 2*2*BLOCK_MULTIPLE*BLOCK_MULTIPLE*sizeof(int_32_t));
  currMB->cbp_bits = cs->cbp_bits;

}

