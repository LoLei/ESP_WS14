//-----------------------------------------------------------------------------
// assa.c
//
// Trajectory of projectile
//
// Group: 5 study assistant Philipp Hafner
//
// Authors:
// Lorenz Leitner 1430211
// Stefan Bräuer 1330690
// Verena Niederwanger 14300778
// Julian Lanca-Gil 1430212
//
// Latest Changes: 09.12.2014 (by Stefan Bräuer)
//-----------------------------------------------------------------------------
//

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MSG_PARAMETER "usage: ./assa [float:angle] [float:speed] "\
"[output_filename] {optional:config_filename}\n"
#define MSG_OOM "error: out of memory\n"
#define MSG_WRITE "error: couldn't write file\n"
#define MSG_SPEED "error: speed must be > 0\n"
#define MSG_CONFIG "no config file found - using default values\n"


#define TYPE 19778
#define BITS_PER_PIXEL 24
#define PLANES 1
#define COMPRESSION 0
#define X_PIXEL_PER_METER 0x130B //2835 , 72 DPI
#define Y_PIXEL_PER_METER 0x130B //2835 , 72 DPI
#define LINE_STRENGTH 5


#pragma pack(push,1)
typedef struct
{
  unsigned int height;
  unsigned int width;
  unsigned int pps;
  float gravitation;
  float wind_angle;
  float wind_force;
  float v_angle;
  float v_speed;
} Parameter;

typedef struct
{
  uint16_t signature;
  uint32_t file_size;
  uint32_t reserved;
  uint32_t fileoffset_to_pixelarray;
} FileHeader;

typedef struct
{
  uint32_t dib_header_size;
  uint32_t width;
  uint32_t height;
  uint16_t planes;
  uint16_t bits_per_pixel;
  uint32_t compression;
  uint32_t image_size;
  uint32_t y_pixel_per_meter;
  uint32_t x_pixel_per_meter;
  uint32_t num_colors_pallette;
  uint32_t most_imp_color;
} BitMapInfoHeader;

//Eines der beiden all_lower_case
typedef struct
{
  FileHeader file_header;
  BitMapInfoHeader bit_map_info_header;
} BitMap;
#pragma pack(pop)


BitMap* createHeader(Parameter *params, BitMap *pbitmap)
{
  int pixel_byte_size = params->height * params->width * BITS_PER_PIXEL/8;
  int file_size = pixel_byte_size + sizeof(BitMap);
  pbitmap->file_header.signature = TYPE;
  pbitmap->file_header.file_size = file_size;
  pbitmap->file_header.fileoffset_to_pixelarray = sizeof(BitMap);
  pbitmap->bit_map_info_header.dib_header_size = sizeof(BitMapInfoHeader);
  pbitmap->bit_map_info_header.width = params->width;
  pbitmap->bit_map_info_header.height = params->height;
  pbitmap->bit_map_info_header.planes = PLANES;
  pbitmap->bit_map_info_header.bits_per_pixel = BITS_PER_PIXEL;
  pbitmap->bit_map_info_header.compression = COMPRESSION;
  pbitmap->bit_map_info_header.image_size = pixel_byte_size;
  pbitmap->bit_map_info_header.y_pixel_per_meter = Y_PIXEL_PER_METER;
  pbitmap->bit_map_info_header.x_pixel_per_meter = X_PIXEL_PER_METER;
  pbitmap->bit_map_info_header.num_colors_pallette = 0;
  return pbitmap;
}

int readConfig(char *file_name, Parameter *params)
{
  FILE *cfile = NULL;
  if((cfile = fopen(file_name, "r")) == NULL)
  {
    printf(MSG_CONFIG);
    return 0;
  }
  char propname[20];
  float tempwidth = 0;
  int unused_wind = 1;
  int unused_res = 1;
  int unused_pps = 1;
  int unused_grav = 1;
  float prop = 0;
  int errors = 0;
  while(!feof(cfile))
  {
    fscanf(cfile, "%s", propname);
    if(strcmp(propname, "wind") == 0 && unused_wind)
    {
      if(!feof(cfile) && fscanf(cfile, "%f", &prop) != 0)
      {
        params->wind_angle = prop;
        printf("wind set to %.2f degrees", params->wind_angle);
        if(!feof(cfile) && (fscanf(cfile, "%f", &prop)) != 0)
        {
          params->wind_force = prop;
          printf(" with force %.2fm/s^2\n", params->wind_force);
        }
        else
          printf("\n");
      }
      else
        errors++;
      unused_wind = 0;
    }
    else if(strcmp(propname, "resolution") == 0 && unused_res)
    {
      if(!feof(cfile) && fscanf(cfile, "%f", &prop) != 0 && prop == (int)prop)
      {
        tempwidth = (int) prop;
        if(!feof(cfile) && (fscanf(cfile, "%f", &prop)) != 0 &&
            prop == (int)prop)
        {
          params->width = tempwidth;
          params->height = (int) prop;
          printf("resolution set to %d:%d\n", params->width, params->height);
        }
        else
          errors++;
      }
      else
        errors++;
      unused_res = 0;
    }
    else if(strcmp(propname, "pps") == 0 && unused_pps)
    {
      if(!feof(cfile) && fscanf(cfile, "%f", &prop) != 0 && prop == (int)prop)
      {
        if(prop > 0)
        {
          params->pps = (int) prop;
          printf("pps set to %d\n", params->pps);
        }
      }
      else
        errors++;
      unused_pps = 0;
    }
    else if(strcmp(propname, "gravitation") == 0 && unused_grav)
    {
      if(!feof(cfile) && fscanf(cfile, "%f", &prop) != 0)
      {
        params->gravitation = prop;
        printf("gravitation set to %.2fm/s^2\n", params->gravitation);
      }
      else
        errors++;
      unused_grav = 0;
    }
  }
  errors = errors + unused_grav + unused_pps + unused_res + unused_wind;
  if(errors)
    printf("%d missing or incorrect entrie(s) - using default values\n",
        errors);
  fclose(cfile);
  return 0;
}

int drawBackground(int color, Parameter *params,
    int (*pixel_buffer)[params->height])
{

  int curheight = 0;
  int curwidth = 0;

  for(curwidth = 0; curwidth < params->width; curwidth++)
    for(curheight = 0; curheight < params->height; curheight++)
      pixel_buffer[curwidth][curheight] = color;
  return 0;
}

int drawLine(int **points, int point_count, int str, int color,
    Parameter *params, int (*pixel_buffer)[params->height])
{
  int cur_point = 0;
  int cur_line = 0;
  int x_str = 0;
  int y_str = 0;
  int x_dif = 0;
  int y_dif = 0;
  int x_poi = 0;
  int y_poi = 0;
  int change = 0;

  if(!str)
    str = LINE_STRENGTH;

  for(cur_point = 0; cur_point < point_count-1; cur_point++)
  {
    x_dif = (points[0][cur_point+1] - points[0][cur_point]);
    y_dif = (points[1][cur_point+1] - points[1][cur_point]);
    if((y_dif*y_dif) >= (x_dif*x_dif))
    {
      change = (y_dif < 0) ? -1 : 1;
      for(cur_line = 0; cur_line != y_dif; (cur_line += change))
        for(x_str = 0; x_str < str; x_str++)
        {
          x_poi = points[0][cur_point] + ((x_dif*cur_line)/y_dif) - str/2 +
              x_str;
          for(y_str = 0; y_str < str; y_str++)
          {
            y_poi = points[1][cur_point] + cur_line - str/2 + y_str;
            if(!(y_poi < 0 || y_poi > params->height || x_poi < 0 ||
                x_poi > params->width))
            {
              pixel_buffer[x_poi][y_poi] = color;
            }
          }
        }
    }
    else
    {
      change = (x_dif < 0) ? -1 : 1;
      for(cur_line = 0; cur_line != x_dif; (cur_line += change))
        for(y_str = 0; y_str < str; y_str++)
        {
          y_poi = points[1][cur_point] + ((y_dif*cur_line)/x_dif) - str/2 +
              y_str;
          for(x_str = 0; x_str < str; x_str++)
          {
            x_poi = points[0][cur_point] + cur_line - str/2 + x_str;
            if(!(y_poi < 0 || y_poi > params->height || x_poi < 0 ||
                x_poi > params->width))
            {
              pixel_buffer[x_poi][y_poi] = color;
            }
          }
        }
    }
  }

  return 0;
}

int drawRectangle(int **points, int color, Parameter *params,
    int (*pixel_buffer)[params->height])
{
  int curwidth = 0;
  int curheight = 0;
  int changewidth = 0;
  int changeheight = 0;

  changewidth = (points[0][0] > points[0][1]) ? -1 : 1;
  changeheight = (points[1][0] > points[1][1]) ? -1 : 1;

  for(curheight = points[1][0]; curheight != (points[1][1] + changeheight);
      curheight += changeheight)
  {
    for(curwidth = points[0][0]; curwidth != (points[0][1] + changewidth);
        curwidth += changewidth)
    {
      if(!(curheight < 0 || curheight >= params->height || curwidth < 0 ||
          curwidth >= params->width))
      {
        pixel_buffer[curwidth][curheight] = color;
      }

    }

  }
  return 0;
}

int drawCannon(Parameter *params, int (*pixel_buffer)[params->height])
{

  int **points;
  points = (int **) malloc(sizeof(int) * 4);
  points[0] = (int *)realloc(NULL, sizeof(int)*2);
  points[1] = (int *)realloc(NULL, sizeof(int)*2);
  points[0][0] = 0;
  points[0][1] = params->width;
  points[1][0] = params->width/2 - cos((90 - params->v_angle) / 57.2957795) *
      params->width/12;
  points[1][1] = 0;
  drawRectangle(points, 0x005000, params, pixel_buffer);
  points[0][1] = params->width/2;
  points[1][1] = params->height/2;

  points[0][0] = points[0][1] - cos(params->v_angle / 57.2957795) *
      params->width/8;
  points[1][0] = points[1][1] - cos((90 - params->v_angle) / 57.2957795) *
      params->width/8;
  drawLine(points, 2, (params->width/24), 0xA0A0A0, params, pixel_buffer);
  points[1][1] = points[1][0] - params->width/24;
  points[0][0] = points[0][0] - params->width/32;
  points[0][1] = points[0][1] + params->width/32;

  drawRectangle(points, 0x705000, params, pixel_buffer);
  points[0][0] = params->width/2 + cos((90 - params->v_angle) / 57.2957795) *
      params->width/38;
  points[1][0] = params->height/2 - cos(params->v_angle / 57.2957795) *
      params->height/64;
  points[0][1] = params->width/2 - cos((90 - params->v_angle) / 57.2957795) *
      params->width/48;
  points[1][1] = params->height/2 + cos(params->v_angle / 57.2957795) *
      params->height/24;
  drawLine(points, 2, (params->width/48), 0xA0A0A0, params, pixel_buffer);


  return 0;
}

int drawBitMap(char *bmp_name, int **points, int *counter, Parameter *params)
{

  int curheight = 0;
  int curwidth = 0;
  int pixel_buffer[params->width][params->height];
  FILE *fp = NULL;
  if((fp = fopen(bmp_name, "wb")) == NULL)
  {
    //Could not write file
    printf(MSG_WRITE);
    return 3;

  }
  BitMap *bmap;
  if((bmap = (BitMap*) calloc(1, sizeof(BitMap))) == NULL)
  {
    printf(MSG_OOM);
    return 2;
  }
  createHeader(params, bmap);
  fwrite(bmap, 1, sizeof(BitMap), fp);
  free(bmap);
  drawBackground(0x60D0FF, params, pixel_buffer);
  drawCannon(params, pixel_buffer);
  drawLine(points, *counter, 0, 0xFF0000, params, pixel_buffer);

  for(curheight = 0; curheight < params->height; curheight++)
    for(curwidth = 0; curwidth < params->width; curwidth++)
      fwrite( &pixel_buffer[curwidth][curheight], 3, 1, fp);
  fclose(fp);
  return 0;
}

int calculation(int **points, int *counter, Parameter *params)
{
  // (first argument = float winkel, second argument = float speed) = velocity vector = v
  // cfg-file: BitMap size, pps = unsigned int pixel per second, float gravitation, float wind_force, float wind_angle
  // v = velocity, t = time, g = gravitation, w = wind
  // 1 pixel = 10 meters


  //force und speed sind in meter angegeben, müssen durch 10 dividiert werden, damit pixel
  float wind_force_pxl = params->wind_force / 10;
  float v_force_pxl = params->v_speed / 10;
  float v_x = (v_force_pxl * cos(params->v_angle / 57.2957795));
  float v_y = (v_force_pxl * cos((90 - params->v_angle) / 57.2957795));
  float p = 1/(float)params->pps;
  float t = p;
  float g = -params->gravitation/10;
  float w_x = (wind_force_pxl * cos(params->wind_angle/ 57.2957795));
  float w_y = (wind_force_pxl * cos((90 - params->wind_angle)/ 57.2957795));
  int cur_x = 1;

  do
  {
    if((cur_x % 10) == 0)
    {
      if((points[0] = (int *) realloc(points[0], (cur_x + 10)*(sizeof(int))))
          == NULL)
      {
        printf(MSG_OOM);
        return 2;
      }
      if((points[1] = (int *) realloc(points[1], (cur_x + 10)*(sizeof(int))))
          == NULL)
      {
        printf(MSG_OOM);
        return 2;
      }
    }
    t = p * cur_x;
    points[0][cur_x] = points[0][0] + v_x * t + w_x * t*t / 2 + 0.5;
    points[1][cur_x] = points[1][0] + v_y * t + g * t*t / 2 + w_y*t*t/ 2 + 0.5;
    cur_x++;
  }

  while(points[0][cur_x-2] < params->width && points[0][cur_x-2] > 0 &&
        points[1][cur_x-2] > 0 && points[1][cur_x-2] < params->height);
  *counter = cur_x;
  return 0;
}

Parameter* setStandard(Parameter *params, float angle, float speed)
{

  params->height = 320;
  params->width = 320;
  params->pps = 5;
  params->gravitation = 9.798;
  params->wind_angle = 0;
  params->wind_force = 0;
  params->v_angle = angle;
  params->v_speed = speed;
  return params;
}

int main(int argc, char *argv[])
{

  if((argc < 4)|(argc > 5))
  {
    printf(MSG_PARAMETER);
    return 1;
  }
  Parameter *params;
  if((params = (Parameter*) calloc(1, sizeof(Parameter))) == NULL)
  {
    printf(MSG_OOM);
    return 2;
  }
  setStandard(params, strtof(argv[1],NULL), strtof(argv[2],NULL));
  if(argc == 5)
  {
    readConfig(argv[4], params);
  }
  else
  {
    printf(MSG_CONFIG);
  }
  char *bmp_name = argv[3];
  int **points;


  if(params->v_speed <= 0)
  {
  printf(MSG_SPEED);
  return 4;
  }

  if((points = (int **) malloc(sizeof(int) * 20)) == NULL)
  {
    printf(MSG_OOM);
    return 2;
  }
  if((points[0] = (int *)realloc(NULL, sizeof(int)*10)) == NULL)
  {
    printf(MSG_OOM);
    return 2;
  }
  if((points[1] = (int *)realloc(NULL, sizeof(int)*10)) == NULL)
  {
    printf(MSG_OOM);
    return 2;
  }
  points[0][0] = params->width / 2;
  points[1][0] = params->height / 2;
  int counter = 0;
  if(calculation(points, &counter, params) == 2)
    return 2;

  int drawreturn = 0;
  if((drawreturn = drawBitMap(bmp_name, points, &counter, params)) == 3)
    return 3;
  else if(drawreturn == 2)
    return 2;
  free(points);

  return 0;
}
