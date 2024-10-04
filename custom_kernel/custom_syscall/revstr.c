#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/linkage.h>
#include <linux/uaccess.h>

SYSCALL_DEFINE2(revstr, char*, input_str, int, n){
  char str[100];
  char temp;
  copy_from_user(str, input_str, n); // (void *to, const void *from, unsigned long n)
  printk("The origin string: %s\n", str);
  
  for(int i = 0; i < n/2; i++){
    temp = str[i];
    str[i] = str[n-1-i];
    str[n-1-i] = temp;
  }
  
  copy_to_user(input_str, str, n);
  printk("The reversed string: %s\n", str);
  
  return 0;
}
