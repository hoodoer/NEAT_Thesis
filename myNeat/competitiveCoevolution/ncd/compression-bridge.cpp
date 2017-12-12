#ifdef LZMA
#include <stdio.h>
#include <stdlib.h>
#include <lzma.h>

#include <complearn/cl-core.h>

// deltaByteLen of 0 means no delta coding
// 1 means each byte delta'd with last
// 2 means every other byte
// 3 means every 3rd
// 4 means every 4th
// 5,6,7,8 are allowed

CLNumber compressed_size_in_bits_and_delta(const char *buf, size_t lenBytes, int deltaByteLen)
{
  lzma_stream strm = LZMA_STREAM_INIT;
  lzma_options_lzma lopts;
  lzma_options_delta dopts;
  lzma_filter filters[3];
  int fstart = 0;
  if (deltaByteLen > 0 && deltaByteLen <= LZMA_DELTA_DIST_MAX) {
    fstart += 1;
    int i;
    char *dop = (char *) &dopts;
    for (i = 0; i < sizeof(dopts); ++i) {
      dop[i] = 0;
    }
    dopts.dist = deltaByteLen;




    filters[0].id = LZMA_FILTER_DELTA;
    filters[0].options = &dopts;
  }
  lzma_lzma_preset(&lopts, 9 | LZMA_PRESET_EXTREME);
  lopts.pb = 0;
  lopts.lc = 0;
  if (lopts.dict_size < lenBytes) {
    lopts.dict_size = lenBytes;
  }

  filters[fstart].id = LZMA_FILTER_LZMA2;
  filters[fstart].options = &lopts;
  filters[fstart+1].id = LZMA_VLI_UNKNOWN;
  filters[fstart+1].options = 0;
  int outbuflen = (lenBytes * 5) / 4 + 2048;
  size_t clen = 0;
  char *outbuf = (char*)malloc(outbuflen);
  int retval =
  lzma_raw_encoder(&strm, filters);
  if (retval != LZMA_OK) {
    fprintf(stderr, "Error, cannot init compression stream: %d\n", retval);
    exit(1);
  }
  strm.next_in = (unsigned char *) buf;
  strm.avail_in = lenBytes;
  strm.next_out = (unsigned char *) outbuf;
  strm.avail_out = outbuflen;
//  retval = lzma_code(&strm, LZMA_RUN);
//  lzma_stream_buffer_encode(filters, LZMA_CHECK_NONE, NULL, buf, lenBytes,
//    outbuf, &clen, outbuflen);
//  if (retval != LZMA_OK) {
//    fprintf(stderr, "Error, cannot compress data: %d\n", retval);
//    exit(1);
//  }
  retval = lzma_code(&strm, LZMA_FINISH);
  if (retval != LZMA_STREAM_END) {
    fprintf(stderr, "Error, cannot finish compressing data: %d\n", retval);
    exit(1);
  }
  clen = (char *) strm.next_out - outbuf;
  free(outbuf);
  lzma_end(&strm);
//  lzma_easy_buffer_encode(9 | LZMA_PRESET_EXTREME, 
  return clen * 8;
}

double compressed_size_in_bits(char *buf, int lenBytes)
{
  int deltasToTry[] = { 0, 1, 2, 3, 4, 8 };
  int i, maxi = sizeof(deltasToTry) / sizeof(deltasToTry[0]);
  double bestSize = lenBytes * 1024 + 1024;
  for (i = 0; i < maxi; ++i) {
    int d = deltasToTry[i];
    double clen = compressed_size_in_bits_and_delta(buf, lenBytes, d);
    if (clen < bestSize) {
      bestSize = clen;
//      printf("Using delta=%d\n", d);
    }
  }
  return bestSize;
}
#endif

