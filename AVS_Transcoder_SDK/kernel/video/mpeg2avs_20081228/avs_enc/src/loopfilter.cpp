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
* File name: 
* Function: 
*
*************************************************************************************
*/

#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "const_data.h"

#define  IClip( Min, Max, Val) (((Val)<(Min))? (Min):(((Val)>(Max))? (Max):(Val)))

extern const int_32_t QP_SCALE_CR[64] ;

byte ALPHA_TABLE[64]  = {
     0, 0, 0, 0, 0, 0, 1, 1,
     1, 1, 1, 2, 2, 2, 3, 3,
     4, 4, 5, 5, 6, 7, 8, 9,
    10,11,12,13,15,16,18,20,
    22,24,26,28,30,33,33,35,
    35,36,37,37,39,39,42,44,
    46,48,50,52,53,54,55,56,
    57,58,59,60,61,62,63,64 
} ;
byte  BETA_TABLE[64]  = {
   0, 0, 0, 0, 0, 0, 1, 1,
   1, 1, 1, 1, 1, 2, 2, 2,
   2, 2, 3, 3, 3, 3, 4, 4,
   4, 4, 5, 5, 5, 5, 6, 6,
   6, 7, 7, 7, 8, 8, 8, 9,
   9,10,10,11,11,12,13,14,
  15,16,17,18,19,20,21,22,
  23,23,24,24,25,25,26,27
};
byte CLIP_TAB[64] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 
  1, 1, 1, 1, 1, 1, 2, 2,
  2, 2, 2, 2, 2, 2, 3, 3,
  3, 3, 3, 3, 3, 4, 4, 4,
  5, 5, 5, 6, 6, 6, 7, 7,
  7, 7, 8, 8, 8, 9, 9, 9
} ;



void GetStrength(byte Strength[2],Macroblock* MbP,Macroblock* MbQ,int_32_t dir,int_32_t edge,int_32_t block_y,int_32_t block_x);
void EdgeLoop(byte* SrcPtr,byte Strength[2],int_32_t QP,  int_32_t dir,int_32_t width,int_32_t Chro);
void DeblockMb(ImageParameters *img, byte **imgY, byte ***imgUV, int_32_t blk_y, int_32_t blk_x) ;

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return: 
* Attention:
*************************************************************************
*/

void c_avs_enc::DeblockFrame(ImageParameters *img, byte **imgY, byte ***imgUV)
{
  int_32_t mb_x, mb_y ;
  img->current_mb_nr = -1;    
  for( mb_y=0 ; mb_y<(img->height>>4) ; mb_y++ )
  {
    for( mb_x=0 ; mb_x<(img->width>>4) ; mb_x++ )
      {
      img->current_mb_nr++;   
      DeblockMb( img, imgY, imgUV, mb_y, mb_x ) ;
      }
  }
} 

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return: 
* Attention:
*************************************************************************
*/

void c_avs_enc::DeblockMb(ImageParameters *img, byte **imgY, byte ***imgUV, int_32_t mb_y, int_32_t mb_x)
{
  int_32_t           EdgeCondition;
  int_32_t           dir, edge, QP ;                                                          
  byte          Strength[2], *SrcY, *SrcU = NULL, *SrcV = NULL;
  Macroblock    *MbP, *MbQ ; 
  const int_32_t mb_width = input->img_width/MB_BLOCK_SIZE;
  Macroblock *currMB = &img->mb_data[img->current_mb_nr];

  SrcY = imgY[mb_y<<4] + (mb_x<<4) ;    // pointers to source
  if (imgUV != NULL)
  {
    SrcU = imgUV[0][mb_y<<3] + (mb_x<<3) ;
    SrcV = imgUV[1][mb_y<<3] + (mb_x<<3) ;
  }
  MbQ  = &img->mb_data[mb_y*(img->width>>4) + mb_x] ;      // current Mb

  if (MbQ->lf_disable) return;
  for( dir=0 ; dir<2 ; dir++ )                            // vertical edges, than horicontal edges
  {
    EdgeCondition = (dir && mb_y) || (!dir && mb_x)  ;  // can not filter beyond frame boundaries
    if(dir && mb_y)
    {
      EdgeCondition = (currMB->slice_nr == img->mb_data[img->current_mb_nr-mb_width].slice_nr)? EdgeCondition:0;  //  can not filter beyond slice boundaries
    }
    for( edge=0 ; edge<2 ; edge++ )                                            // first 4 vertical strips of 16 pel
    {                                                                          // then  4 horizontal
      if( edge || EdgeCondition )
      {
        MbP = (edge)? MbQ : ((dir)? (MbQ -(img->width>>4))  : (MbQ-1) ) ;    // MbP = Mb of the remote 4x4 block   MbP�Ƿ���Ҫ���µõ���
        QP = (MbP->qp + MbQ->qp+1) >> 1 ;                                   // Average QP of the two blocks
        GetStrength( Strength, MbP, MbQ, dir, edge, mb_y<<2, mb_x<<2);           //Strength for 4 pairs of blks in 1 stripe
        if( *((int_16_t*)Strength) )  // && (QP>= 8) )                    // only if one of the 4 Strength bytes is != 0
        { 
          EdgeLoop( SrcY + (edge<<3)* ((dir)? img->width:1 ), Strength,QP,dir, img->width, 0 );
          if( (imgUV != NULL) && !(edge & 1) )
          {
            EdgeLoop( SrcU +  (edge<<2) * ((dir)? img->width_cr:1 ), Strength,QP_SCALE_CR[QP], dir, img->width_cr, 1 ) ; 
            EdgeLoop( SrcV +  (edge<<2) * ((dir)? img->width_cr:1 ), Strength, QP_SCALE_CR[QP], dir, img->width_cr, 1 ) ; 
          }
        }
      }
    }//end edge
  }//end loop dir
}

byte BLK_NUM[2][2][2]  = {{{0,8},{2,10}},{{0,2},{8,10}}} ;
/*
*************************************************************************
* Function:
* Input:
* Output:
* Return:
* Attention:
*************************************************************************
*/
void c_avs_enc::GetStrength(byte Strength[2],Macroblock* MbP,Macroblock* MbQ,int_32_t dir,int_32_t edge,int_32_t block_y,int_32_t block_x)
{
  int_32_t    blkQ, idx ;
  int_32_t    blk_x2, blk_y2, blk_x, blk_y ;
  for( idx=0 ; idx<2 ; idx++ )
  {
    blkQ = BLK_NUM[dir][edge][idx] ;
    blk_y = block_y + (blkQ>>2);
    blk_x = block_x + (blkQ&3);
    blk_x >>= 1;
    blk_x += 4;
    blk_y >>= 1;
    blk_y2 = blk_y -  dir ;  
    blk_x2 = blk_x - !dir ;

    if((MbP->mb_type==I4MB) || (MbQ->mb_type==I4MB))
      Strength[idx] = 2;

    else if(img->type==INTER_IMG)
      Strength[idx] = (abs(tmp_mv[0][blk_y][blk_x] -   tmp_mv[0][blk_y2][blk_x2]) >= 4 ) ||
      (abs( tmp_mv[1][blk_y][blk_x] -   tmp_mv[1][blk_y2][blk_x2]) >= 4) ||
      (refFrArr [blk_y][blk_x-4] !=   refFrArr[blk_y2][blk_x2-4]);
    else 
      Strength[idx] = (abs( tmp_fwMV[0][blk_y][blk_x] - tmp_fwMV[0][blk_y2][blk_x2]) >= 4) ||
      (abs( tmp_fwMV[1][blk_y][blk_x] - tmp_fwMV[1][blk_y2][blk_x2]) >= 4) ||
      (abs(tmp_bwMV[0][blk_y][blk_x]  - tmp_bwMV[0][blk_y2][blk_x2]) >= 4) ||
      (abs( tmp_bwMV[1][blk_y][blk_x] - tmp_bwMV[1][blk_y2][blk_x2]) >= 4) || 
      (fw_refFrArr [blk_y][blk_x-4] !=   fw_refFrArr[blk_y2][blk_x2-4]) || 
      (bw_refFrArr [blk_y][blk_x-4] !=   bw_refFrArr[blk_y2][blk_x2-4]);
  }
}

/*
*************************************************************************
* Function:
* Input:
* Output:
* Return: 
* Attention:
*************************************************************************
*/
void c_avs_enc::EdgeLoop(byte* SrcPtr,byte Strength[2],int_32_t QP, int_32_t dir,int_32_t width,int_32_t Chro)
{
  int_32_t      pel, PtrInc, Strng ;
  int_32_t      inc, inc2, inc3, inc4 ;
  int_32_t      C0,  Delta, dif, AbsDelta ;
  int_32_t      L2, L1, L0, R0, R1, R2, RL0 ;
  int_32_t      Alpha, Beta ;
  int_32_t       ap, aq, small_gap;

  PtrInc  = dir?  1 : width ;
  inc     = dir ?  width : 1 ;                    
  inc2    = inc<<1 ;    
  inc3    = inc + inc2 ;    
  inc4    = inc<<2 ;

  Alpha = ALPHA_TABLE[Clip3(0,63,QP+input->alpha_c_offset)];

  Beta = BETA_TABLE[Clip3(0,63,QP+input->beta_offset)];

  for( pel=0 ; pel<16 ; pel++ )
  {
    if( (Strng = Strength[pel >> 3]) )
    {
      L2  = SrcPtr[-inc3] ;
      L1  = SrcPtr[-inc2] ;
      L0  = SrcPtr[-inc ] ;
      R0  = SrcPtr[    0] ;
      R1  = SrcPtr[ inc ] ;
      R2  = SrcPtr[ inc2] ;
      AbsDelta = abs(Delta = R0 - L0) ;
      if( AbsDelta < Alpha )
      {
        if((abs(R0 - R1) < Beta) && (abs(L0 - L1) < Beta))
        {
          aq = (abs( R0 - R2) < Beta) ;
          ap = (abs( L0 - L2) < Beta) ;
          if(Strng == 2)
          {
            RL0 = L0 + R0 ;
            small_gap = (AbsDelta < ((Alpha >> 2) + 2));
            aq &= small_gap;
            ap &= small_gap;

            SrcPtr[   0 ] = aq ? ( R1 + RL0 +  R0 + 2) >> 2 : ((R1 << 1) + R0 + L0 + 2) >> 2 ;
            SrcPtr[-inc ] = ap ? ( L0 +  L1 + RL0 + 2) >> 2 : ((L1 << 1) + L0 + R0 + 2) >> 2 ;

            SrcPtr[ inc ] = (aq && !Chro) ? ( R0 + R1 + L0 + R1 + 2) >> 2 : SrcPtr[ inc ];
            SrcPtr[-inc2] = (ap && !Chro) ? ( L1 + L1 + L0 + R0 + 2) >> 2 : SrcPtr[-inc2];
          }
          else  //Strng == 2
          {
            C0  = CLIP_TAB[Clip3(0,63,QP+input->alpha_c_offset)];   // jlzheng 7.12
            dif             = IClip( -C0, C0, ( (R0 - L0) * 3 + (L1 - R1) + 4) >> 3 ) ;
            SrcPtr[  -inc ] = IClip(0, 255, L0 + dif) ;
            SrcPtr[     0 ] = IClip(0, 255, R0 - dif) ;
            if( !Chro )
            {
              L0 = SrcPtr[-inc] ;
              R0 = SrcPtr[   0] ;
              if(ap)
              {
                dif           = IClip( -C0, C0, ( (L0 - L1) * 3 + (L2 - R0) + 4) >> 3 ) ;
                SrcPtr[-inc2] = IClip(0, 255, L1 + dif) ;
              }
              if(aq)
              {
                dif           = IClip( -C0, C0, ( (R1 - R0) * 3 + (L0 - R2) + 4) >> 3 ) ;
                SrcPtr[ inc ] = IClip(0, 255, R1 - dif) ;
              }
            }
          }
        }
      }
      SrcPtr += PtrInc ;
      pel    += Chro ;
    }
    else                      //���û���˲�ǿ�ȣ������˲��������ñߵ���һ��
    {
      SrcPtr += PtrInc << (3 - Chro) ;
      pel    += 7 ;
    }  ;
  }
}
