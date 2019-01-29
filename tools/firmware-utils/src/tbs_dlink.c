
#include "tbs_dlink.h"

int ConvertEndian(int val,int endian)
{
  int i;
  unsigned char *p;
  unsigned char *q;

  i = val;
  p = ( unsigned char * ) &val;
  q = ( unsigned char * ) &i;

  if(endian == BIGENDIAN)
  {
    *q = *(p+3);
    *(q+1) = *(p+2);
    *(q+2) = *(p+1);
    *(q+3) = *p;
  }

  return i;
}

int tbs_crc_file(FILE *fp , unsigned int offset , unsigned long *checksum)
{
  unsigned char buf[BUFLEN];
  unsigned long crc = 0;
  unsigned int length = 0;
  unsigned int bytes_read;
  
  fseek(fp, offset , SEEK_SET);

  while((bytes_read = fread(buf, 1, BUFLEN, fp)) > 0)
  {
    unsigned char *cp = buf;

    if(length + bytes_read < length)
      return 0;

    length += bytes_read;
    while(bytes_read--)
      crc =(crc << 8) ^ crctab[((crc >> 24) ^ *cp++) & 0xFF];
  }
 
  if(ferror(fp))
    return 0;

  for(; length; length >>= 8)
    crc =(crc << 8) ^ crctab[((crc >> 24) ^ length) & 0xFF];

  crc = ~crc & 0xFFFFFFFF;
  *checksum = crc;

  return 1;
}

int CreateImgFile(const unsigned char *outimg)
{
  char *img_orig;
  update_hdr_t  image_header;

  FILE    *pfin;
  FILE    *pfout;

  int     iImgFileLength = 0;      /* count how many bytes have been write to IMG file */
  int     iReadCount;
  int     iWriteCount;
  int     tmp;
  int     i;
  unsigned long checksum_result;
    
  memset(&image_header,0,sizeof(update_hdr_t));

  strcpy( image_header.product, TBS_PRODUCT);
  strcpy( image_header.version, TBS_PRODVERSION);
  strcpy( image_header.img_type, TBS_IMGTYPE);
  strcpy( image_header.board_id, TBS_BOARDID);

  //for netgear
  strcpy( image_header.region, TBS_REGION);
  strcpy( image_header.model_name,  TBS_MODEL_NAME);
  strcpy( image_header.swversion,  TBS_SWVERSION);

  image_header.kernel_size = 0x1D0000;
  image_header.rootfs_size = 0x45E000;

  img_orig = (unsigned char *)malloc(image_header.kernel_size+image_header.rootfs_size);
  memset(img_orig, 0xffff, image_header.kernel_size+image_header.rootfs_size);

  pfin = fopen(outimg,"rb");
  if(pfin == NULL)
  {
    printf("Can't open image file: %s\n",outimg);
    return 1;
  }

  iWriteCount=0;
  while(!feof(pfin)) {
    iReadCount = fread(img_orig+iWriteCount,1,4096,pfin);
    iWriteCount += iReadCount;
  }

  fclose(pfin);

  pfout = fopen(outimg,"wb+");
  if(pfout == NULL)
  {
    printf("Can't open output file: %s\n",outimg);
    return 1;
  } 

  for(i=0; i<sizeof(update_hdr_t); i++)
  {
    fputc(0xffff,pfout);
    iImgFileLength++;
  }

  image_header.kernel_offset = iImgFileLength;
  image_header.rootfs_offset = image_header.kernel_size + image_header.kernel_offset;
  image_header.image_len = image_header.kernel_size+image_header.rootfs_size;

  fwrite(img_orig, 1, image_header.kernel_size+image_header.rootfs_size, pfout);
  free(img_orig);

/******************************************************************
        Deal with image header            
******************************************************************/

  image_header.rootfs_offset = ConvertEndian(image_header.rootfs_offset, LITTLEENDIAN);
  image_header.rootfs_size = ConvertEndian(image_header.rootfs_size, LITTLEENDIAN);
  image_header.kernel_offset = ConvertEndian(image_header.kernel_offset, LITTLEENDIAN);
  image_header.kernel_size = ConvertEndian(image_header.kernel_size, LITTLEENDIAN);
  image_header.image_len = ConvertEndian(image_header.image_len, LITTLEENDIAN);

  if( tbs_crc_file(pfout , sizeof(update_hdr_t) , &checksum_result ) )
  {
    image_header.image_checksum = ConvertEndian( checksum_result , LITTLEENDIAN);

    if(fseek(pfout,0,SEEK_SET) == -1)
    {
          printf("Fail to point image header.\n");
          fclose(pfout);
          remove(outimg);
          return 1;                         /* fail to lseek */
    }

    iWriteCount = fwrite(&image_header,sizeof(update_hdr_t),1,pfout);
    if(iWriteCount != 1)  /* fail to write ? */
    {
         printf("Fail to write checksum to IMG file.\n");
         fclose(pfout);
         remove(outimg);
         return 1;
    }
  }


/******************************************************************
        set image.img file crc           
******************************************************************/
  
  tbs_crc_file( pfout , 0 , &checksum_result );
  checksum_result = ConvertEndian( checksum_result, LITTLEENDIAN);

  if(fseek(pfout,0,SEEK_END) == -1)
  {
    printf("Fail to point IMG file tail.\n");
    fclose(pfout);
    remove(outimg);
    return 1;                         /* fail to lseek */
  }

  if(fwrite( &checksum_result , 1, 4 , pfout )  <  4 )
  {
    printf("Fail to write file_checksum to IMG file.\n");
    fclose(pfout);
    remove(outimg);
    return 1;
  }
  
  fclose(pfout);
  
  return 0;
}

int main(int argc, char *argv[])
{
  return CreateImgFile(argv[1]);
}
