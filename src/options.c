#include "game_main.h"

#ifdef PLATFORM_PLAYDATE

#include "pd_api.h"
extern PlaydateAPI *playdate;

#define fopen(name, mode)                                                      \
  (playdate->file->open(name, (mode[0] == 'w' ? kFileWrite : kFileRead)))
#define fclose(File) playdate->file->close(File)
#define fread(buf, len, cz, File) playdate->file->read(File, buf, (len * cz))
#define fwrite(buf, len, cz, File) playdate->file->write(File, buf, (len * cz))
#define ftell(File) (playdate->file->tell(File))
#define fseek(File, pos, whence) playdate->file->seek(File, pos, whence)
#define fgets(buf, len, File) _readLineFileFunction(File, buf, len)

char *_readLineFileFunction(void *fp, void *buf, unsigned int len) {
  PlaydateAPI *pd = playdate;
  if (!fp) {
    return 0;
  }

  char *buffer = (char *)buf;
  int idx = 0;

  char c;
  while (pd->file->read(fp, &c, 1) == 1) { // Read one byte (char) at a time
    if (c == '\n' || c == '\r') {          // Stop at newline or carriage return
      buffer[idx++] = 0;
      break;
    }
    buffer[idx++] = c;
    if (idx == len)
      break;
  }

  if (idx == 0)
    return NULL;

  buffer[idx] = 0;
  return buf;
}
#endif

// saves the data structure to disk
void save_options(options_t *o, const char *filename) {
#if 1
  char *basePath = platform_dirname(platform_executable_path());
  char tmpPath[1024];
  sprintf(tmpPath, "%s%s", basePath, filename);

  FILE *file = fopen(tmpPath, "w");
  char line[1024];
  sprintf(line, "level_pack = %d\n", o->level_pack);
  fwrite(line, sizeof(char), strlen(line), file);
  sprintf(line, "level = %d\n", o->level);
  fwrite(line, sizeof(char), strlen(line), file);
  for (int i = 0; i < 5; i++) {
    sprintf(line, "max_level_%02d = %d\n", i, o->max_levels[i]);
    printf("%s\n", line);
    fwrite(line, sizeof(char), strlen(line), file);
  }
  fclose(file);
#endif
}

// loads the data structure from disk
bool load_options(options_t *o, const char *filename) {
  char *basePath = platform_dirname(platform_executable_path());
  char tmpPath[1024];
  sprintf(tmpPath, "%s%s", basePath, filename);

  memset(o, 0, sizeof(options_t));

  FILE *file = fopen(tmpPath, "r");
  if (!file)
    return false;

  char buf[1024];
  while (fgets(buf, 1024, file)) {
    char *token = strtok(buf, "=");
    if (token != NULL) {
      char name[32];
      strcpy(name, token);
      float fp[] = {0, 0, 0, 0, 0, 0, 0, 0};
      int fidx = 0;
      token = strtok(NULL, " ");
      while (token != NULL) {
        float number;
        if (sscanf(token, "%f", &number) == 1) {
          fp[fidx++] = number;
        } else {
          printf("Error parsing float: %s\n", token);
        }
        token = strtok(NULL, " ");
      }

      if (strstr(name, "level_pack") == name) {
        o->level_pack = fp[0];
      }
      if (strstr(name, "level") == name) {
        o->level = fp[0];
      }
      if (strstr(name, "max_level_") == name) {
        char num[] = {name[10], name[11], 0};
        int n = atoi(num);
        if (strstr(name, "cherries") != NULL) {
          o->max_levels[n] = (int)fp[0];
        }
      }
    }
  }

  fclose(file);
  return true;
}