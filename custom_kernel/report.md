##### 作業說明：https://hackmd.io/@-ZZtFnnqSZ2F1A-Uy-GMlw/HJJj4BcKC
##### 報告網址：https://hackmd.io/@iTkWej8WRj2Xrnb85FtlPQ/HJHK4MpC0

## I. Compiling the Linux Kernel

### 1. 下載所需套件
```git
$ sudo apt update
$ sudo apt-get install git libncurses5-dev libncurses-dev libssl-dev build-essential bison flex libelf-dev
```

### 2. 取得 kernel source 
```terminal
git clone https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git –depth=1 -b v6.1 –single-branch
```

### 3. 查看資料夾資訊 
```
cd linux
```
```
git log
```
![1_init_git_log](https://hackmd.io/_uploads/rkydqfpCA.png)
```
head Makefile -n 5
```
![2_head_Makefile](https://hackmd.io/_uploads/SyUMjz60R.png)

### 4. 編譯、建置 kernel
```
sudo make distclean O=build
```
#### 修改 Kernel Configuration
> 把terminal視窗放到最大

```
make menuconfig
``` 

> “General setup” ->  “Local version - append to kernel release”

![4_menuconfig](https://hackmd.io/_uploads/HkhfCGTA0.png)

#### 修改`.config`中的下列參數 (使用UI操作須顯示隱藏檔)
```
CONFIG_SYSTEM_TRUSTED_KEYS=""
CONFIG_SYSTEM_REVOCATION_KEYS=""
```
#### 建置 kernel
```
sudo make -j4 (or sudo make -j$(nproc))
sudo make -j4 modules_install 
sudo make -j4 install
```

#### 更新 GRUB
- 讓 GRUB 選單自動開啟、儲存(optional)
    > 到 `/etc/default/grub`修改以下資訊
    ```
    GRUB_SAVEDEFAULT=true
    GRUB_TIMEOUT_STYLE=menu
    GRUB_TIMEOUT=-1
    ```

```
sudo update-grub
```

### 5. 進入建置的 kernel 版本
#### 重新開機，進入 GRUB 選單 (按下shift)
> “Advanced options for ubuntu”-> 選版本

![7_GRUB](https://hackmd.io/_uploads/B1iZRGpCA.png)

### 6. 確認版本
```
uname -a
cat /etc/os-release
```
![9_version_data](https://hackmd.io/_uploads/SyvqaG6CC.png)

---

## II. Implementing a new System Call

### 1. 創建資料夾、新增 system call 程式碼
```
cd linux
mkdir custom_syscall
cd custom_syscall
```
#### 創建 system call 檔案、編寫程式碼
```
touch revstr.c
```
- revstr.c 內容：
```C
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
```
#### 建立 custom_syscall 的 Makefile
```
touch Makefile
```
- Makefile 內容：
```
obj-y += revstr.o 
```

### 2.修改其他檔案
#### 回 linux 資料夾
```
cd ../
```

#### 修改 Kbuild
-  新增`obj-y += custom_syscall/`

#### 修改 arch/x86/entry/syscalls/syscall_64.tbl
- 在system call區域最後新增```451	common	revstr	sys_revstr```

#### 修改 include/linux/syscalls.h
- 在#endif前新增一行`asmlinkage int sys_revstr(char* input_str, int n);`

### 3. 回 linux 資料夾，編譯、安裝 kernel
```
sudo make clean
sudo make -j$(nproc)
sudo make -j$(nproc) modules_install install
```

### 4. 進入建置的 kernel 版本
#### 重新開機，進入 GRUB 選單，選擇 kernel (6.1.0-os-313551156+)

### 5. 測試 system call
#### 建立測試用程式碼
```
touch test_revstr.c
```
- test_revstr.c 內容：
```C
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
    
#define __NR_revstr 451
    
int main(int argc, char *argv[]) {
    
    char str1[20] = "hello";
    printf("Ori: %s\n", str1);
    int ret1 = syscall(__NR_revstr, str1, strlen(str1));
    assert(ret1 == 0);
    printf("Rev: %s\n", str1);

    char str2[20] = "Operating System";
    printf("Ori: %s\n", str2);
    int ret2 = syscall(__NR_revstr, str2, strlen(str2));
    assert(ret2 == 0);
    printf("Rev: %s\n", str2);

    return 0;
}
```
#### 編譯、執行 test_revstr.c
```
gcc test_revstr.c
./a.out
```
![8_test_revstr](https://hackmd.io/_uploads/S1_jmQ60A.png)

#### 查看 kernel ring buffer
```
sudo dmesg
```
![5_dmesg](https://hackmd.io/_uploads/rk2q7XpAA.png)

---

## III. Patch
### 1. 設定 git config、commit
```
git status
git config --global user.email "a0935262880@gmail.com"
git config --global user.name "Shawn"
git commit -m "add the system call - revstr"
git log
```
![3_git_log](https://hackmd.io/_uploads/r1pg4QaCA.png)

### 2. 製作 patch
```
git format-patch –root -o ./patch
```
![6_patch](https://hackmd.io/_uploads/rJi44QTCC.png)

---

## IV. Package list
### 1. 列出下載的 packages
```
dpkg --get-selections > packages_list_313551156.txt
```
