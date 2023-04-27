#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"

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

void execute_cmd(char *input) {
  // execute command from input 
  if (strcmp(input, "clear") == 0) { 
    // clear screen
    // clear_screen();
    syscall_user(7, 0, 0, 0);
    // framebuffer_set_cursor(0, 0);
    syscall_user(6, 0, 0, 0);
    syscall_user(5, (uint32_t) "> ", 256, 0xF);
  } else {
    if (strcmp(input, "help") == 0) {
      // list of available commands
      syscall_user(5, (uint32_t) "\nAvailable commands:\n", 256, 0xF);
      syscall_user(5, (uint32_t) "clear - Clear the screen\n", 256, 0xF);
      syscall_user(5, (uint32_t) "help - List of available commands", 256, 0xF);
    } else if (strcmp(input, "rm") == 0) {
      syscall_user(5, (uint32_t) "\nDeleting file: ", 256, 0xF);
      syscall_user(5, (uint32_t) input, 256, 0xF);
    } else {
      syscall_user(5, (uint32_t) "\nCommand not found: ", 256, 0xF);
      syscall_user(5, (uint32_t) input, 256, 0xF);
    }
    // framebuffer_set_cursor(c.row + 1, 0);
    syscall_user(6, 1, 0, 0);
    syscall_user(5, (uint32_t) "> ", 256, 0xF);
  } 
}

int main(void) {
    // struct ClusterBuffer cl[5];
    // struct FAT32DriverRequest request = {
    //     .buf                   = &cl,
    //     .name                  = "daijoubu",
    //     .ext                   = "owo",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size           = 5*CLUSTER_SIZE,
    // };
    // int32_t retcode;
    // syscall_user(0, (uint32_t) &request, (uint32_t) &retcode, 0);
    // syscall_user(5, retcode, 4, 0xF);
    // if (retcode == 0){
    //     syscall_user(5, (uint32_t) "owo\n", 4, 0xF);
    // }

    char buf[256];
    while (TRUE) {
        syscall_user(4, (uint32_t) buf, 256, 0); // apakah ini 
        execute_cmd(buf);
        // syscall_user(5, (uint32_t) buf, 256, 0xF);
        // framebuffer_write_string(buf, 0x0F);
    }
    
    

    return 0;
}
