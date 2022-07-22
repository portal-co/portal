#include <cerrno>
#include <cstdio>
#include <cstdlib>
#define _GNU_SOURCE
#include <fcntl.h>
#include <fstream>
#include <sched.h>
#include <string>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
int main(int argc, char **argv) {
  std::vector<char *> a(argv + 1, argv + argc);
  a.push_back(0);
  int *status = (int *)mmap(0, sizeof(int), PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, 0, 0);
  *status = 0;
  int f = fork();
  if (f) {
    while (*status == 0)
      ;
    *status = 2;
    int s;
    while (wait(&s) != f)
      ;
    return WEXITSTATUS(s);
  }
  auto us = getuid();
  auto g = getgid();
  // fprintf(stderr," - gid: %d\n",g);
  int t2 = us;
  while (unshare(CLONE_NEWUSER) != 0)
    printf("- Cannot unshare namespace\n");
  {
    auto u = std::string("/proc/") + std::to_string(getpid()) + "/uid_map";
    auto o = open(u.c_str(), O_RDWR);
    std::string z = "0 ";
    std::string t = z + std::to_string(us) + " 1\n";
    while (write(o, t.c_str(), t.size()) != -1)
      printf("- Cannot add user namespace, retrying\n");
    close(o);
  }
  {
    {
      std::ofstream p("/proc/self/setgroups");
      p << "deny";
    }
    auto u = std::string("/proc/") + std::to_string(getpid()) + "/gid_map";
    auto o = open(u.c_str(), O_RDWR);
    std::string z = "0 ";
    std::string t = z + std::to_string(g) + " 1\n";
    while (write(o, t.c_str(), t.size()) != -1)
      printf("- Cannot add user namespace, retrying\n");
    close(o);
  }
  while (getuid() != 0 || getgid() != 0) {
    int b = setegid(0);
    int d = errno;
    int a = seteuid(0);
    int c = errno;
    printf("- UID: %d, GID: %d (ERR: %d,%d # %d,%d) \n", getuid(), getgid(), a,
           b, c, d);
    if (getuid() != 0 || getgid() != 0)
      printf("- Cannot be root, retrying\n");
  }
  while (unshare(CLONE_NEWNS | CLONE_NEWPID) != 0)
    printf("- Cannot unshare namespace\n");
  *status = 1;
  while (*status == 1)
    ;
  int f2 = fork();
  if (f2) {
    int s;
    while (wait(&s) != f)
      ;
    return WEXITSTATUS(s);
  }
  while (mount("x", "/proc", "proc", 0, 0) != 0)
    printf("- Cannot mount proc\n");
  if (getuid() != 0 || getgid() != 0) {
    printf("- Not root\n");
    return 1;
  }
  execvp(a[0], &a[0]);
}