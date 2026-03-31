// rename conflicting 7zip symbols to avoid clashing with unarc's LZMA
#define LzmaDec_Allocate      SZ_LzmaDec_Allocate
#define LzmaDec_AllocateProbs SZ_LzmaDec_AllocateProbs
#define LzmaDec_DecodeToBuf   SZ_LzmaDec_DecodeToBuf
#define LzmaDec_DecodeToDic   SZ_LzmaDec_DecodeToDic
#define LzmaDec_Free          SZ_LzmaDec_Free
#define LzmaDec_FreeProbs     SZ_LzmaDec_FreeProbs
#define LzmaDec_Init          SZ_LzmaDec_Init
#define LzmaDecode            SZ_LzmaDecode
#define LzmaProps_Decode      SZ_LzmaProps_Decode
#define MyAlloc               SZ_MyAlloc
#define MyFree                SZ_MyFree
