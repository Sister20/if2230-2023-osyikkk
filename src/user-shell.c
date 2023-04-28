#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"

#define BLACK 0x00
#define DARK_BLUE 0x01
#define DARK_GREEN 0x2
#define DARK_AQUA 0x3
#define DARK_RED 0x4
#define DARK_PURPLE 0x5
#define GOLD 0x6
#define GRAY 0x7
#define DARK_GRAY 0x8
#define BLUE 0x09
#define GREEN 0x0A
#define AQUA 0x0B
#define RED 0x0C
#define LIGHT_PURPLE 0x0D
#define YELLOW 0x0E
#define WHITE 0x0F
#define KEYBOARD_BUFFER_SIZE 256

// TODO: Change max stack
uint32_t DIR_NUMBER_STACK [256] = {2};
char DIR_NAME_STACK [256][9] = {"ROOT\0\0\0\0\0"};
uint8_t DIR_STACK_LENGTH = 1;

void syscall_user(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

int strcmp(char *s1, char *s2) {
  int i = 0;
  while (s1[i] == s2[i]) {
    if (s1[i] == '\0') {
      return 0;
    }
    i++;
  }
  return s1[i] - s2[i];
}

void string_combine(char* s1, char* s2, char* res) {
    int i = 0;
    while (s1[i] != '\0') {
        res[i] = s1[i];
        i++;
    }
    int j = 0;
    while (s2[j] != '\0') {
        res[i+j] = s2[j];
        j++;
    }
    res[i+j] = '\0';
}

void print_home() {
  char home [256];
  char base [9] = "OSyikkk:\0";
  char path [2] = "/\0";

  for(int i = 0; i < DIR_STACK_LENGTH; i++) {
    if(i == 0) {
      string_combine(base, path, home);
    } else {
      string_combine(home, path, home);
      string_combine(home, DIR_NAME_STACK[i], home);
    }
  }

  syscall_user(5, (uint32_t) home, KEYBOARD_BUFFER_SIZE, BLUE);
  syscall_user(5, (uint32_t) "> ", KEYBOARD_BUFFER_SIZE, WHITE); 
}

void change_directory(char* new_dir) {
  struct FAT32DirectoryTable req_table;
  struct FAT32DriverRequest request = {
      .buf                   = &req_table,
      .parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH - 1],
      .ext                   = "\0\0\0",
      .buffer_size           = 0,
  };

  if(strcmp(new_dir, "..") == 0) {
    if(DIR_STACK_LENGTH <= 1) {
      return;
    } else {
      DIR_STACK_LENGTH--;
    }
  } else if (strcmp(new_dir, ".") == 0) {
    return;
  } else {    
    for(int i = 0; i < 8; i++) {
      request.name[i] = new_dir[i];
    }

    int8_t retcode;
    syscall_user(1, (uint32_t) &request, (uint32_t) &retcode, 0);
    
    if(retcode != 0) {
      syscall_user(5, (uint32_t) "INVALID DIRECTORY\n", 18, DARK_GREEN);
      return;
    }

    for(int j = 0; j < 8; j++) {
      DIR_NAME_STACK[DIR_STACK_LENGTH][j] = request.name[j];
    }
    DIR_NAME_STACK[DIR_STACK_LENGTH][8] = '\0';
    DIR_NUMBER_STACK[DIR_STACK_LENGTH] = req_table.table[0].cluster_high << 16 | req_table.table[0].cluster_low;
    DIR_STACK_LENGTH++;
  }
}

void list_current_directory() {
  struct FAT32DirectoryTable current_table;
  struct FAT32DriverRequest request = {
      .buf                   = &current_table,
      .ext                   = "\0\0\0",
      .buffer_size           = 0,
  };

  for(int i = 0; i < 8; i++) {
    request.name[i] = DIR_NAME_STACK[DIR_STACK_LENGTH - 1][i];
  }

  if(DIR_STACK_LENGTH <= 1) {
    request.parent_cluster_number = ROOT_CLUSTER_NUMBER;
  } else {
    request.parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH - 2];
  }
  
  int8_t retcode;
  syscall_user(1, (uint32_t) &request, (uint32_t) &retcode, 0);

  if(retcode != 0) {
    syscall_user(5, (uint32_t) "SHELL ERROR\n", 6, DARK_GREEN);
  }

  for(uint32_t i = 1; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
    if(current_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
      char filename[9];
      for(int j = 0; j < 8; j++) {
        filename[j] = current_table.table[i].name[j];
      }
      filename[8] = '\0';

      if(current_table.table[i].attribute == ATTR_ARCHIVE) {
        syscall_user(5, (uint32_t) filename, 8, WHITE);

        if(current_table.table[i].ext[0] != '\0') {
          char ext_name[4];
          for(int j = 0; j < 3; j++) {
            ext_name[j] = current_table.table[i].ext[j];
          }
          ext_name[3] = '\0';

          syscall_user(5, (uint32_t) ".", 1, WHITE);
          syscall_user(5, (uint32_t)ext_name, 3, WHITE);
        }
      } else {
        syscall_user(5, (uint32_t) filename, 8, AQUA);
      }

      syscall_user(5, (uint32_t) " ", 1, WHITE);
    }
  }
}

// TODO: Not yet checked
void remove(char* target_filename, char* target_extension) {
  struct FAT32DirectoryTable current_table;
  struct FAT32DriverRequest request = {
      .buf                   = &current_table,
      .ext                   = "\0\0\0",
      .buffer_size           = 0,
  };

  for (int i = 0; i < 8; i++) {
    request.name[i] = DIR_NAME_STACK[DIR_STACK_LENGTH - 1][i];
  }

  if(DIR_STACK_LENGTH <= 1) {
    request.parent_cluster_number = ROOT_CLUSTER_NUMBER;
  } else {
    request.parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH - 2];
  }
  
  int8_t retcode;
  syscall_user(1, (uint32_t) &request, (uint32_t) &retcode, 0);

  if (retcode != 0) {
    syscall_user(5, (uint32_t) "SHELL ERROR\n", 15, DARK_GREEN);
    return;
  }


  for (uint32_t i = 1; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
    if (current_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
      char filename[9];
      for (int j = 0; j < 8; j++) {
        filename[j] = current_table.table[i].name[j];
      }
      filename[8] = '\0';

      if (current_table.table[i].attribute == ATTR_ARCHIVE) {
        char ext_name[4] = "\0\0\0\0";
        if (current_table.table[i].ext[0] != '\0') {
          for (int j = 0; j < 3; j++) {
            ext_name[j] = current_table.table[i].ext[j];
          }
          ext_name[3] = '\0';
        }

        if (strcmp(filename, target_filename) == 0 && strcmp(ext_name, target_extension) == 0) {
          
          struct FAT32DriverRequest request_delete = {
            .parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH - 1],
          };
  
          for (int j = 0; j < 8; j++) {
            request_delete.name[j] = filename[j];
          }

          for (int j = 0; j < 3; j++) {
            request_delete.ext[j] = ext_name[j];
          }
          
          int8_t retcode_delete;
          syscall_user(3, (uint32_t) &request_delete, (uint32_t) &retcode_delete, 0);
          return;
        }
      }


    }
  }
  
  
}

// TODO: Implement where_is
// void where_is(uint32_t* path_number_stack, char** path_name_stack, uint8_t stack_length) {
//   struct FAT32DirectoryTable req_table;
//   struct FAT32DriverRequest request = {
//       .buf                   = &req_table,
//       .ext                   = "\0\0\0",
//       .buffer_size           = 0,
//   };
//   for(int i = 0; i < 8; i++) {
//     request.name[i] = path_name_stack[stack_length - 1][i];
//   }
  
//   if(stack_length <= 1) {
//     request.parent_cluster_number = ROOT_CLUSTER_NUMBER;
//   } else {
//     request.parent_cluster_number = path_number_stack[stack_length - 2];
//   }

//   int8_t retcode;
//   syscall_user(1, (uint32_t) &request, (uint32_t) &retcode, 0);

//   if(retcode != 0) {
//     syscall_user(5, (uint32_t) "SHELL ERROR\n", 6, DARK_GREEN);
//   }

//   for(uint32_t i = 1; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
//     if(current_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
//       char filename[9];
//       for(int j = 0; j < 8; j++) {
//         filename[j] = current_table.table[i].name[j];
//       }
//       filename[8] = '\0';

//       if(current_table.table[i].attribute != ATTR_ARCHIVE) {
//         if(strcmp(filename, path_name_stack[stack_length - 1]) == 0) {
//           char path[256];

//           for(int i = 0; i < DIR_STACK_LENGTH; i++) {
//             if(i == 0) {
//               string_combine(path, path, path);
//             } else {
//               string_combine(path, path, path);
//               string_combine(path, DIR_NAME_STACK[i], path);
//             }
//           }
//           syscall_user(5, (uint32_t) home, KEYBOARD_BUFFER_SIZE, WHITE);
//         }
        
//         path_number_stack[stack_length] = req_table.table[i].cluster_high << 16 | req_table.table[i].cluster_low;
//         for(int j = 0; j < 8; j++) {
//           path_name_stack[stack_length][j] = filename[j];
//         }
//         stack_length++;

//       }
//     }
//   }
// }

void clear_screen(){
  syscall_user(7, 0, 0, 0);
  syscall_user(6, 0, 0, 0);
  print_home();
  // syscall_user(5, (uint32_t) "> ", 256, 0xF);
}

// nigel.exe
// target_name = nigel\0\0\0
// target_ext = exe

void parse_file_cmd(char* file, char* target_name, char* target_ext) {
  for (int i = 0; i < 9; i++) {
    target_name[i] = '\0';
  }
  
  for (int i = 0; i < 3; i++) {
    target_ext[i] = '\0';
  }
  uint8_t ext_found = 0;
  int idx_target_name = 0;
  int idx_target_ext = 0;
  
  for (int idx = 0; idx < 12; idx++) {
    if (file[idx] == '.') {
      ext_found = 1;
      // idx++;
      continue;
    }
    if (ext_found == 0) {
      if (idx_target_name > 7) {
        syscall_user(5, (uint32_t) "File/dir name too long (max 8)", KEYBOARD_BUFFER_SIZE, DARK_GREEN);
        return;
      }
      target_name[idx_target_name] = file[idx]; 
      idx_target_name++;
    } else {
      if (idx_target_ext > 3) {
        syscall_user(5, (uint32_t) "File extension too long (max 3)", KEYBOARD_BUFFER_SIZE, DARK_GREEN);
        return;
      }
      target_ext[idx_target_ext] = file[idx];
      idx_target_ext++;
    }
  }
}

void execute_cmd(char *input, char* home) {
  // TODO : untuk rm, cat, cp extensioonnya dipecah setelah cd cd an
  // remove space in the first character
  while (input[0] == ' ') {
    for (int i = 0; i < 256; i++) {
      input[i] = input[i+1];
      
    }
  }

  // array of string input
  char cmd[40][15];

  // 
  int cmd_length = 0;

  // initialize array with \0
  for (int i = 0; i < 40; i++) {
    for (int j = 0; j < 15; j++) {
      cmd[i][j] = '\0'; 
    }
  }
  int neff = 0;
  int i = 0;
  while (input[i] != '\0') {
    int j = 0;
    while (input[i] != ' ' && input[i] != '\0' && input[i] != '/') {
      cmd[neff][j] = input[i];
      j++;
      i++;
    }
    
    cmd[neff][j] = '\0';
    neff++;

    if (input[i] == ' ' ) {
      cmd[neff][0] = ' ';
      neff++;
      cmd_length++;
      while (input[i] == ' ') {
        i++;
      }
    }
    
    if (input[i] == '/') {
      i++;
    }
  }

  // for (int i = 0; i < neff; i++) {
  //   syscall_user(5, (uint32_t) cmd[i], KEYBOARD_BUFFER_SIZE, WHITE);
  //   syscall_user(5, (uint32_t) "\n", KEYBOARD_BUFFER_SIZE, WHITE);
  // }
  // cp tep adflasd f     /al/sdf/s -> ini dari root

  // syscall_user(5, (uint32_t) "\n", KEYBOARD_BUFFER_SIZE, WHITE);
  // syscall_user(5, (uint32_t) target_name, KEYBOARD_BUFFER_SIZE, WHITE);
  // syscall_user(5, (uint32_t) target_ext, KEYBOARD_BUFFER_SIZE, WHITE);
  
  char targetn[9];
  char targetext[3];
  char tergetFull[12] = "test\0\0\0\0.exe";
  parse_file_cmd(tergetFull, targetn, targetext);
  
  // execute command from input 
  if (strcmp(cmd[0], "clear") == 0) { 
    clear_screen();
  } else {
    syscall_user(5, (uint32_t) "\n", 1, WHITE);

    if (strcmp(cmd[0], "help") == 0) {
      // list of available commands
      syscall_user(5, (uint32_t) "Available commands:\n", KEYBOARD_BUFFER_SIZE, WHITE);
      syscall_user(5, (uint32_t) "clear - Clear the screen\n", KEYBOARD_BUFFER_SIZE, WHITE);
      syscall_user(5, (uint32_t) "help - List of available commands", KEYBOARD_BUFFER_SIZE, WHITE);
    } else if(strcmp(cmd[0], "cd") == 0) {
      // cd : Pindah ke folder yang dituju
      if (cmd[1][0] == '\0') {
        // cd
        // change_directory(home);
        DIR_STACK_LENGTH = 1;
      }

      int i = 2;
      while (cmd[i][0] != '\0') {
        if (cmd[i][0] == '.') {
          if (cmd[i][1] == '.') {
            // cd ..
            change_directory("..");
          } else {
            // cd .
            change_directory(".");
          }
        } else {
          // cd <folder>
          change_directory(cmd[i]);
        }
        i++;
      }
      // change_directory(cmd[1]);
      
    } else if(strcmp(cmd[0], "ls") == 0 ) {
      list_current_directory();
    } else if(strcmp(cmd[0], "mkdir") == 0) {
        // mkdir : Membuat sebuah folder kosong baru
        int counterCD = 2;
        if (cmd[2][0]=='\0'){
          syscall_user(5, (uint32_t) "mkdir: missing operand\n", 31, WHITE);
        }
        else {
          // Change to target directory
          uint32_t DIR_NUMBER_STACK_TEMP [256] = {2};
          char DIR_NAME_STACK_TEMP [256][9] = {"ROOT\0\0\0\0\0"};
          uint8_t DIR_STACK_LENGTH_TEMP = DIR_STACK_LENGTH;
          for (int i = 0; i < DIR_STACK_LENGTH+1; i++){
            DIR_NUMBER_STACK_TEMP[i] = DIR_NUMBER_STACK[i];
            for (int j = 0; j < 9; j++){
              DIR_NAME_STACK_TEMP[i][j] = DIR_NAME_STACK[i][j];
            }
          }
          if (cmd[3][0] != '\0'){
            counterCD = 2;
            while (cmd[counterCD+1][0] != '\0') {
              if (cmd[counterCD][0] == '.') {
                if (cmd[counterCD][1] == '.') {
                  // cd ..
                  change_directory("..");
                } else {
                  // cd .
                  change_directory(".");
                }
              } else {
                // cd <folder>
                change_directory(cmd[counterCD]);
              }
              counterCD++;
            }
          }

          // mkdir file in relative path
          struct FAT32DirectoryTable current_table;
          struct FAT32DriverRequest request = {
              .buf                   = &current_table,
              .name                  = "\0\0\0\0\0\0\0\0",
              .ext                   = "\0\0\0",
              .parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH-1],
              .buffer_size           = 0,
          };
          // char temp[8];
          for(int i = 0; i < 8; i++) {
            request.name[i] = cmd[counterCD][i];
            // temp[i] = cmd[counterCD][i];
          }
          int8_t retcode;
          // syscall_user(5, (uint32_t) temp, 8, WHITE);
          syscall_user(2, (uint32_t) &request, (uint32_t) &retcode, 0);
          if(retcode != 0) {
            syscall_user(5, (uint32_t) "INVALID DIRECTORY", 18, DARK_GREEN);
          } 

          // change to previous stack
          for (int i = 0; i < DIR_STACK_LENGTH_TEMP; i++){
            DIR_NUMBER_STACK[i] = DIR_NUMBER_STACK_TEMP[i];
            for (int j = 0; j < 9; j++){
              DIR_NAME_STACK[i][j] = DIR_NAME_STACK_TEMP[i][j];
            }
          }
          for (int i = DIR_STACK_LENGTH_TEMP; i < (DIR_STACK_LENGTH_TEMP+counterCD); i++){
            DIR_NUMBER_STACK[i] = 0;
            for (int j = 0; j < 9; j++){
              DIR_NAME_STACK[i][j] = '\0';
            }
          }
          DIR_STACK_LENGTH = DIR_STACK_LENGTH_TEMP;
        }
        
        
    } else if(strcmp(cmd[0], "cat") == 0) {
        // cat : Menuliskan sebuah file sebagai text file ke layar (Gunakan format LF newline)
        
        int counterCD = 2;
        if (cmd[2][0]=='\0'){
          syscall_user(5, (uint32_t) "cat: missing operand\n", 29, WHITE);
        }
        else{
          // Change to target directory
          uint32_t DIR_NUMBER_STACK_TEMP [256] = {2};
          char DIR_NAME_STACK_TEMP [256][9] = {"ROOT\0\0\0\0\0"};
          uint8_t DIR_STACK_LENGTH_TEMP = DIR_STACK_LENGTH;
          for (int i = 0; i < DIR_STACK_LENGTH+1; i++){
            DIR_NUMBER_STACK_TEMP[i] = DIR_NUMBER_STACK[i];
            for (int j = 0; j < 9; j++){
              DIR_NAME_STACK_TEMP[i][j] = DIR_NAME_STACK[i][j];
            }
          }
          if (cmd[3][0] != '\0'){
            counterCD = 2;
            while (cmd[counterCD+1][0] != '\0') {
              if (cmd[counterCD][0] == '.') {
                if (cmd[counterCD][1] == '.') {
                  // cd ..
                  change_directory("..");
                } else {
                  // cd .
                  change_directory(".");
                }
              } else {
                // cd <folder>
                change_directory(cmd[counterCD]);
              }
              counterCD++;
            }
          }

          // parse file name
          char full_name[12];
          for (int i=0;i<12;i++){
            full_name[i] = cmd[counterCD][i];
          }
          char file_name[9];
          char file_ext[3];
          parse_file_cmd(full_name, file_name, file_ext);
          // if (cmd[counterCD][8] == '.'){
          //   for (int i=0;i<3;i++){
          //     file_ext[i] = cmd[counterCD][i+9];
          //   }
          // }
          // else{
          //   syscall_user(5, (uint32_t) "cat: invalid file name\n", 24, WHITE);
          // }
          // if (cmd[counterCD][12] != '\0' || cmd[counterCD][12] != ' '){
          //   syscall_user(5, (uint32_t) "cat: invalid file name\n", 24, WHITE);
          // }

          
          // parse_file_cmd(full_name, file_name, file_ext);
          
          // cat file in relative path
          struct FAT32DirectoryTable current_table;
          struct FAT32DriverRequest request = {
              .buf                   = &current_table,
              .ext                   = "\0\0\0",
              .buffer_size           = 0,
          };
          
          if(DIR_STACK_LENGTH <= 1) {
            request.parent_cluster_number = ROOT_CLUSTER_NUMBER;
          } else {
            request.parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH - 2];
          }
          
          int8_t retcode;
          for (int i=0;i<8;i++){
            request.name[i] = DIR_NAME_STACK[DIR_STACK_LENGTH-1][i];
          }
          // for (int i=0; i<3; i++){
          //   request.ext[i] = file_ext[i];
          // }
          syscall_user(1, (uint32_t) &request, (uint32_t) &retcode, 0);
          if (retcode != 0 ){
            
          }
          for(uint32_t i = 1; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
            char curr_name [9];
            for(int j = 0; j < 8; j++) {
              curr_name[j] = current_table.table[i].name[j];
            }
            curr_name[8] = '\0';

            if(strcmp(curr_name, file_name) == 0) {
              // char extension [4];
              // for(int j = 0; j < 3; j++) {
              //   extension[j] = current_table.table[i].ext[j];
              // }
              // extension[3] = '\0';
                  
              uint32_t size = current_table.table[i].filesize;
              struct ClusterBuffer cl[size/CLUSTER_SIZE];
              struct FAT32DriverRequest requestBuf = {
                  .buf                   = &cl,
                  .parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH - 1],
                  .buffer_size           = size,
              };
              for (int j = 0; j < 8; j++) {
                requestBuf.name[j] = cmd[counterCD][j];
              }
              for (int j = 0; j < 3; j++) {
                requestBuf.ext[j] = file_ext[j];
              }
              syscall_user(0, (uint32_t) &requestBuf, (uint32_t) &retcode, 0);
              if (retcode!=0){
                char error[50];
                string_combine("cat: ", cmd[1], error);
                string_combine(error, ": No such file or directory\n", error);
                syscall_user(5, (uint32_t) error, 50, WHITE);
              }
              else{
    
                // char fileIn[current_table.filesize];
                // syscall_user(5, (uint32_t) "udah", 5, WHITE);
                for (uint32_t j = 0; j < requestBuf.buffer_size/CLUSTER_SIZE; j++){
                  // fileIn[i] =
                  char temp[CLUSTER_SIZE + 1];
                  for (int k = 0; k < CLUSTER_SIZE; k++) {
                    temp[k] = ((char*) (((struct ClusterBuffer*) requestBuf.buf) + j))[k];
                  }
                  temp[CLUSTER_SIZE] = '\0';
                  syscall_user(5, (uint32_t) temp, 1, WHITE);
                  // syscall_user(5, (uint32_t) "masuk", 5, WHITE);
                }
              }
              break;
            }
          } 

          // change to previous stack
          for (int i = 0; i < DIR_STACK_LENGTH_TEMP; i++){
            DIR_NUMBER_STACK[i] = DIR_NUMBER_STACK_TEMP[i];
            for (int j = 0; j < 9; j++){
              DIR_NAME_STACK[i][j] = DIR_NAME_STACK_TEMP[i][j];
            }
          }
          for (int i = DIR_STACK_LENGTH_TEMP; i < (DIR_STACK_LENGTH_TEMP+counterCD); i++){
            DIR_NUMBER_STACK[i] = 0;
            for (int j = 0; j < 9; j++){
              DIR_NAME_STACK[i][j] = '\0';
            }
          }
          DIR_STACK_LENGTH = DIR_STACK_LENGTH_TEMP;
        }

    } else if(strcmp(cmd[0], "cp") == 0) {
        // cp : Mengcopy suatu file (Folder menjadi bonus)
        // format : cp <file> <tujuan>

        // cek apakah ada argumen dan ada tepat 2 argumen
        if (cmd[1][0]=='\0' || cmd[2][0]=='\0' || cmd[3][0]!='\0'){
          syscall_user(5, (uint32_t) "cp: No such file or directory1\n", KEYBOARD_BUFFER_SIZE, WHITE);
        } else {
          // cek apakah file yang mau dicopy ada
          // ?: parent cluster yang file
          struct FAT32DirectoryTable current_table;
          struct FAT32DriverRequest request = {
              .buf                   = &current_table,
              .ext                   = "\0\0\0",
              .parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH - 1],
              .buffer_size           = 0,
          };

          for (int i=0;i<8;i++){
            request.name[i]=cmd[1][i];
          }
          for (int i=0;i<3;i++){
            request.ext[i]=cmd[1][i+9];
          }

          int8_t retcode;
          syscall_user(1, (uint32_t) &request, (uint32_t) &retcode, 0);

          if (retcode!=0){
            // file
            syscall_user(5, (uint32_t) "cp: No such file or directory2\n", KEYBOARD_BUFFER_SIZE, WHITE);
          } else {
            // file founded
            // check if directory does exist
            struct FAT32DirectoryTable current_table2;
            
            struct FAT32DriverRequest request2 = {
                .buf                   = &current_table2,
                .ext                   = "\0\0\0",
                .parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH - 1],
                .buffer_size           = 0,
            };

            for (int i=0;i<8;i++){
              request2.name[i]=cmd[2][i];
            }
            for (int i=0;i<3;i++){
              request2.ext[i]=cmd[2][i+9];
            }

            int8_t retcode2;
            syscall_user(1, (uint32_t) &request2, (uint32_t) &retcode2, 0);

            if (retcode2!=0) {
              syscall_user(5, (uint32_t) "cp: No such file or directory3\n", KEYBOARD_BUFFER_SIZE, WHITE);
            } else {
              // folder founded
              // copy all content of file to folder
              // get file size

              // request.parent_cluster_number = cluster_number;
              // request.buffer_size =

              syscall_user(2, (uint32_t) &request, (uint32_t) &retcode, 0);

              // check if error or filename is same
              if (retcode!=0){
                syscall_user(5, (uint32_t) "cp: No such file or directory4\n", KEYBOARD_BUFFER_SIZE, WHITE);
              } else {
                ;
                // success
                // syscall_user(5, (uint32_t) "cp: Success\n", 13, WHITE);
              }
            }
          }
        }
               
    } else if(strcmp(cmd[0], "mv") == 0) {
        // mv : Memindah dan merename lokasi file/folder

    } else if(strcmp(cmd[0], "whereis") == 0) {
        // whereis	- Mencari file/folder dengan nama yang sama diseluruh file system
    
    } else if(strcmp(cmd[0], "echo") == 0) {
        // echo : Menuliskan string ke layar 
        syscall_user(5, (uint32_t) cmd[1], 1, WHITE);
        
    } else if (strcmp(cmd[0], "rm") == 0) {
        // rm		- Menghapus suatu file (Folder menjadi bonus)
      int counterCD = 0;
      if (cmd[0][1]=='\0'){
        syscall_user(5, (uint32_t) "rm: missing operand\n", 29, WHITE);
      }
      else{
        syscall_user(5, (uint32_t) "\nDeleting file: ", KEYBOARD_BUFFER_SIZE, WHITE);
        
        // Change to target directory
        uint32_t DIR_NUMBER_STACK_TEMP [256] = {2};
        char DIR_NAME_STACK_TEMP [256][9] = {"ROOT\0\0\0\0\0"};
        uint8_t DIR_STACK_LENGTH_TEMP = DIR_STACK_LENGTH;
        for (int i = 0; i < DIR_STACK_LENGTH+1; i++){
          DIR_NUMBER_STACK_TEMP[i] = DIR_NUMBER_STACK[i];
          for (int j = 0; j < 9; j++){
            DIR_NAME_STACK_TEMP[i][j] = DIR_NAME_STACK[i][j];
          }
        }
        if (cmd[2][0] != '\0'){
          counterCD = 1;
          while (cmd[counterCD+1][0] != '\0') {
            if (cmd[counterCD][0] == '.') {
              if (cmd[counterCD][1] == '.') {
                // cd ..
                change_directory("..");
              } else {
                // cd .
                change_directory(".");
              }
            } else {
              // cd <folder>
              change_directory(cmd[counterCD]);
            }
            counterCD++;
          }
        }
        
        char full_target[12];
        for (int i = 0; i < 12; i++) {
          full_target[i] = cmd[counterCD][i];
        }

        char target_name[9];
        char target_ext[3];
        parse_file_cmd(full_target, target_name, target_ext);
        remove(target_name, target_ext);

        // change to previous stack
        for (int i = 0; i < DIR_STACK_LENGTH_TEMP; i++){
          DIR_NUMBER_STACK[i] = DIR_NUMBER_STACK_TEMP[i];
          for (int j = 0; j < 9; j++){
            DIR_NAME_STACK[i][j] = DIR_NAME_STACK_TEMP[i][j];
          }
        }
        for (int i = DIR_STACK_LENGTH_TEMP; i < (DIR_STACK_LENGTH_TEMP+counterCD); i++){
          DIR_NUMBER_STACK[i] = 0;
          for (int j = 0; j < 9; j++){
            DIR_NAME_STACK[i][j] = '\0';
          }
        }
        DIR_STACK_LENGTH = DIR_STACK_LENGTH_TEMP;
        
        
        
        // int8_t retcode;
        // syscall_user(3, (uint32_t) &request, (uint32_t) &retcode, 0);

      }
    } else {
      syscall_user(5, (uint32_t) "\nCommand not found: ", KEYBOARD_BUFFER_SIZE, DARK_RED);
      syscall_user(5, (uint32_t) input, KEYBOARD_BUFFER_SIZE, DARK_RED);
    }
    syscall_user(6, 1, 0, 0);
    // syscall_user(5, (uint32_t) "> ", KEYBOARD_BUFFER_SIZE, WHITE);
    print_home(home);
  } 
}

int main(void) {    
    char base [9] = "OSyikkk:\0";
    char path [256] = "/\0";
    // char curr_dir [8] = "ROOT\0\0\0\0";

    char home [264];
    string_combine(base, path, home);

    print_home(home);

    char buf[KEYBOARD_BUFFER_SIZE];
    while (TRUE) {
        syscall_user(4, (uint32_t) buf, KEYBOARD_BUFFER_SIZE, 0); 
        execute_cmd(buf, home);
    }

    return 0;
}
