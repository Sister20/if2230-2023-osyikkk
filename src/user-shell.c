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
  char path [2] = "\0";

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

    struct FAT32DirectoryTable parent_table;
    struct FAT32DriverRequest parent_request = {
        .buf                   = &parent_table,
        .ext                   = "\0\0\0",
        .buffer_size           = 0,
    };

    for(int i = 0; i < 8; i++) {
      parent_request.name[i] = DIR_NAME_STACK[DIR_STACK_LENGTH - 1][i];
    }

    if(DIR_STACK_LENGTH <= 1) {
      parent_request.parent_cluster_number = ROOT_CLUSTER_NUMBER;
    } else {
      parent_request.parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH - 2];
    }

    syscall_user(1, (uint32_t) &parent_request, (uint32_t) &retcode, 0);
    if(retcode != 0) {
      syscall_user(5, (uint32_t) "SHELL ERROR\n", 100, DARK_GREEN);
      return;
    }

    for(int j = 0; j < 8; j++) {
      DIR_NAME_STACK[DIR_STACK_LENGTH][j] = request.name[j];
    }
    DIR_NAME_STACK[DIR_STACK_LENGTH][8] = '\0';
    DIR_STACK_LENGTH++;

    for(uint32_t i = 1; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
      char curr_name [9];
      for(int j = 0; j < 8; j++) {
        curr_name[j] = parent_table.table[i].name[j];
      }
      curr_name[8] = '\0';

      if(strcmp(curr_name, DIR_NAME_STACK[DIR_STACK_LENGTH - 1]) == 0) {
        uint32_t curr_cluster = parent_table.table[i].cluster_high << 16 | parent_table.table[i].cluster_low;
        
        DIR_NUMBER_STACK[DIR_STACK_LENGTH-1] = curr_cluster;
      }
    }
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

  for(uint32_t i = 0; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
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

void clear_screen(){
  syscall_user(7, 0, 0, 0);
  syscall_user(6, 0, 0, 0);
  print_home();
  // syscall_user(5, (uint32_t) "> ", 256, 0xF);
}

void execute_cmd(char *input, char* home) {
  // remove space in the first character
  while (input[0] == ' ') {
    for (int i = 0; i < 256; i++) {
      input[i] = input[i+1];
    }
  }
  // array of string input
  char cmd[40][10];

  // initialize array with \0
  for (int i = 0; i < 40; i++) {
    for (int j = 0; j < 10; j++) {
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

    while(input[i] == ' ') {
      i++;
    }
    
    if (input[i] == '/') {
      i++;
    }
  }
  
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
      int i = 1;
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
        if (cmd[1][0]=='\0'){
          syscall_user(5, (uint32_t) "mkdir: missing operand\n", 31, WHITE);
        }
        // else {
        //   struct FAT32DirectoryTable current_table;
        //   struct FAT32DriverRequest request = {
        //       .buf                   = &current_table,
        //       .ext                   = "\0\0\0",
        //       .parent_cluster_number = DIR_NUMBER_STACK[DIR_STACK_LENGTH - 1],
        //       .buffer_size           = 0,
        //   };
        //   for(int i = 0; i < 8; i++) {
        //     request.name[i] = cmd[1][i];
        //   }
        //   int8_t retcode;
        //   syscall_user(1, (uint32_t) &request, (uint32_t) &retcode, 0);

        //   if(retcode != 0) {
        //     syscall_user(5, (uint32_t) "INVALID DIRECTORY", 18, DARK_GREEN);
        //   } else {
        //     for(int i = 0; i < 8; i++) {
        //       dir_name[i] = request.name[i];
        //     }
        //     string_combine(home, request.name, home);
        //   }
        
        
    } else if(strcmp(cmd[0], "cat") == 0) {
        // cat : Menuliskan sebuah file sebagai text file ke layar (Gunakan format LF newline)
        if (cmd[1][0]=='\0'){
          syscall_user(5, (uint32_t) "cat: missing operand\n", 29, WHITE);
        }
        else{
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
          int8_t retcode;
          syscall_user(0, (uint32_t) &request, (uint32_t) &retcode, 0);
          if (retcode!=0){
            char error[50];
            string_combine("cat: ", cmd[1], error);
            string_combine(error, ": No such file or directory\n", error);
            syscall_user(5, (uint32_t) error, 50, WHITE);
          }
        //   else{
        //     char fileIn[current_table.filesize];
        //     for (int i=0;i<current_table.filesize;i++){
        //       fileIn[i]='\0';
        //     }
        //     while (current_table.filesize>0){
        //       char buffer[CLUSTER_SIZE];
        //       struct FAT32DriverRequest request2 = {
        //           .buf                   = buffer,
        //           .ext                   = "\0\0\0",
        //           .parent_cluster_number = *parent_cluster_number,
        //           .buffer_size           = 0,
        //       };
        //       for (int i=0;i<8;i++){
        //         request2.name[i]=cmd[1][i];
        //       }
        //       syscall_user(1, (uint32_t) &request2, (uint32_t) &retcode, 0);
        //       string_combine(fileIn, buffer, fileIn);
        //       // syscall_user(5, (uint32_t) buffer, CLUSTER_SIZE, WHITE);
        //       current_table.filesize-=CLUSTER_SIZE;
        //     }
        //   }
        //   while (fileIn[i]!='\0'){
        //     if (fileIn[i]=='\n'){
        //       syscall_user(5, (uint32_t) "\n", 1, WHITE);
        //     }
        //     else{
        //       syscall_user(5, (uint32_t) &fileIn[i], 1, WHITE);
        //     }
        //     i++;
        //   }
        }
        
    } else if(strcmp(cmd[0], "cp") == 0) {
        // cp : Mengcopy suatu file (Folder menjadi bonus)

        // cek apakah ada argumen dan ada tepat 2 argumen
        if (cmd[1][0]=='\0' || cmd[2][0]=='\0' || cmd[3][0]!='\0'){
          syscall_user(5, (uint32_t) "cp: No such file or directory\n", 31, WHITE);
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
            syscall_user(5, (uint32_t) "cp: No such file or directory\n", 31, WHITE);
          } else {
            
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
      if (cmd[0][1]=='\0'){
        syscall_user(5, (uint32_t) "rm: missing operand\n", 29, WHITE);
      }
      else{
        syscall_user(5, (uint32_t) "\nDeleting file: ", KEYBOARD_BUFFER_SIZE, WHITE);
        // struct FAT32DirectoryTable current_table;
        // struct FAT32DriverRequest request = {
        //     .buf                   = &current_table,
        //     .ext                   = "\0\0\0",
        //     .parent_cluster_number = *parent_cluster_number,
        //     .buffer_size           = 0,
        // };
        // int8_t retcode;
        // syscall_user(3, (uint32_t) &request, (uint32_t) &retcode, 0);

      }
    } else {
      syscall_user(5, (uint32_t) "\nCommand not found: ", KEYBOARD_BUFFER_SIZE, WHITE);
      syscall_user(5, (uint32_t) input, KEYBOARD_BUFFER_SIZE, WHITE);
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
