#include "offload_network.h"
#include "az_types.h"
#include <sys/_types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include "thread.h"
#include "systemcalllist.h"
#include "offload_channel.h"
#include "offload_mmap.h"
#include "offload_message.h"
#include "console.h"
#include "utility.h"
#include "page.h"
#include "memory.h"

struct hostent g_hp;
int hostent_init_flag = 0;

/**
 * @brief Offloading gethostname systemcall that used to access the hostname of the current processor 
 * @param name- the null-terminated hostname in the character array
 * @param len- a length of the character array
 * @return On success, zero is returned. On error, -1 is returned, and errno is set appropriately.
 */
int sys_off_gethostname(char *name, size_t len)
{
  channel_t *ch = NULL;
  struct circular_queue *icq = NULL;
  struct circular_queue *ocq = NULL;

  TCB *current = NULL;
  int mytid = -1;
  int ret = 0;

  // Get the channel for communicating with the driver
  ch = get_offload_channel(-1);
  if(ch == NULL || name == NULL || len == 0)
    return (-1);

  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  // Offload the function to the driver and receive the result
  send_offload_message(ocq, mytid, SYSCALL_sys_gethostname, get_pa((QWORD) name), len, 0, 0, 0, 0);
  ret = (int) receive_offload_message(icq, mytid, SYSCALL_sys_gethostname);

  return ret;
}

/**
 * @brief Offloading gethostbyname systemcall that returns a structure of type hostent for the given host name
 * @param name- a host name. It is either a hostname, or an IPv4 address, or an IPv6 address
 * @return A structure of type hostent 
 */
struct hostent *sys_off_gethostbyname(char *name)
{
  channel_t *ch = NULL;
  struct circular_queue *icq = NULL;
  struct circular_queue *ocq = NULL;

  TCB *current = NULL;
  int mytid = -1;

  // Get the channel for communicating with the driver
  ch = get_offload_channel(-1);
  if(ch == NULL || name == NULL)
    return NULL;

  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  if(hostent_init_flag == 0) {
    g_hp.h_name = (char *) az_alloc(256);
    g_hp.h_aliases[0] = (char *) az_alloc(256);
    g_hp.h_addr = (char *) az_alloc(256);
    if(g_hp.h_name != NULL)
    	lk_memset(g_hp.h_name, 0, 256);
    if(g_hp.h_aliases[0] != NULL)
      lk_memset(g_hp.h_aliases[0], 0, 256);
    if(g_hp.h_addr != NULL)
      lk_memset(g_hp.h_addr, 0, 256);
    hostent_init_flag = 1;
  }

  // Offload the function to the driver and receive the result
  //send_offload_message(ocq, mytid, SYSCALL_sys_gethostbyname, get_pa((QWORD) name), get_pa((QWORD) hp), 0, 0, 0, 0);
  send_offload_message(ocq, mytid, SYSCALL_sys_gethostbyname, get_pa((QWORD) name), get_pa((QWORD) g_hp.h_name), get_pa((QWORD) g_hp.h_aliases[0]), get_pa((QWORD) &(g_hp.h_addrtype)), get_pa((QWORD) &(g_hp.h_length)), get_pa((QWORD) g_hp.h_addr));
  receive_offload_message(icq, mytid, SYSCALL_sys_gethostbyname);

  return &g_hp;
}

/**
 * @brief Offloading getsockname systemcall that returns the current address to which the socket sockfd is bound, in the buffer pointed to by addr
 * @param sockfd- socket descriptor
 * @param addr- the buffer pointer that contains the current address
 * @param addrlen- the amount of space pointed to by addr (in bytes)
 * return On success, zero is returned. On error, -1 is returned, and errno is set appropriately
 */
int sys_off_getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  channel_t *ch = NULL;
  struct circular_queue *icq = NULL;
  struct circular_queue *ocq = NULL;

  TCB *current = NULL;
  int mytid = -1;
  int iret = 0;

  // Get the channel for communicating with the driver
  ch = get_offload_channel(-1);
  if(ch == NULL || addr == NULL || addrlen == NULL)
    return (-1);

  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  // Offload the function to the driver and receive the result
  send_offload_message(ocq, mytid, SYSCALL_sys_getsockname, sockfd, get_pa((QWORD) addr), get_pa((QWORD) addrlen), 0, 0, 0);
  iret = (int) receive_offload_message(icq, mytid, SYSCALL_sys_getsockname);

  return iret;
}

/**
 * @brief Offloading socket systemcall that creates an endpoint for communication and returns a descriptor
 * @param domain- a communication domain
 * @param type- a communication semantics
 * @param protocol- a particular protocol to be used with the socket
 * @return On success, a file descriptor for the new socket is returned. On error, -1 is returned, and errno is set appropriately
 */
int sys_off_socket(int domain, int type, int protocol)
{
  int fd = 0;

  channel_t *ch = NULL;
  struct circular_queue *icq = NULL;
  struct circular_queue *ocq = NULL;

  TCB *current = NULL;
  int mytid = -1;

  // Get the channel for communicating with the driver
  ch = get_offload_channel(-1);
  if(ch == NULL) 
    return (-1);
  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  // Offload the function to the driver and receive the result
  send_offload_message(ocq, mytid, SYSCALL_sys_socket, domain, type, protocol, 0, 0, 0);
  fd = (int) receive_offload_message(icq, mytid, SYSCALL_sys_socket);

  return fd;
}

/**
 * @brief Offloading bind systemcall that assigns the address to the socket referred to by the file descriptor
 * @param sockfd- target file descriptor
 * @param addr- target assigned address
 * @param addrlen- the size of the address structure pointed to by addr
 * @return On success, zero is returned. On error, -1 is returned, and errno is set appropriately
 */
int sys_off_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
  channel_t *ch = NULL;
  struct circular_queue *icq = NULL;
  struct circular_queue *ocq = NULL;

  TCB *current = NULL;
  int mytid = -1;
  int iret = 0;

  // Get the channel for communicating with the driver
  ch = get_offload_channel(-1);
  if(ch == NULL || addr == NULL)
    return (-1);
  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  // Offload the function to the driver and receive the result
  send_offload_message(ocq, mytid, SYSCALL_sys_bind, sockfd, get_pa((QWORD) addr), addrlen, 0, 0, 0);
  iret = (int) receive_offload_message(icq, mytid, SYSCALL_sys_bind);

  return iret;
}

/**
 * @brief Offloading listen systemcall that marks the socket referred to by a file descriptor as a passive socket
 * @param sockfd- a file descriptor that refers to a socket of type SOCK_STREAM or SOCK_SEQPACKET
 * @param backlog- the maximum length to which the queue of pending connections for sockfd may grow
 * @return On success, zero is returned. On error, -1 is returned, and errno is set appropriately
 */
int sys_off_listen(int sockfd, int backlog)
{
  channel_t *ch = NULL;
  struct circular_queue *icq = NULL;
  struct circular_queue *ocq = NULL;

  TCB *current = NULL;
  int mytid = -1;
  int iret = 0;

  // Get the channel for communicating with the driver
  ch = get_offload_channel(-1);
  if(ch == NULL)
    return (-1);
  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  // Offload the function to the driver and receive the result
  send_offload_message(ocq, mytid, SYSCALL_sys_listen, sockfd, backlog, 0, 0, 0, 0);
  iret = (int) receive_offload_message(icq, mytid, SYSCALL_sys_listen);

  return iret;
}

/**
 * @brief Offloading connect systemcall that connects the socket referred to by the file descriptor sockfd to the address specified by addr
 * @param sockfd- a file descriptor
 * @param addr- a target address. The format of the address in addr is determined by the address space of the socket sockfd
 * @param addrlen- the size of addr
 * @return If the connection or binding succeeds, zero is returned. On error, -1 is returned, and errno is set appropriately
 */
int sys_off_connect(int sockfd, struct sockaddr *addr, socklen_t addrlen)
{
  channel_t *ch = NULL;
  struct circular_queue *icq = NULL;
  struct circular_queue *ocq = NULL;

  TCB *current = NULL;
  int mytid = -1;
  int iret = -1;

  // Get the channel for communicating with the driver
  ch = get_offload_channel(-1);
  if(ch == NULL || addr == NULL)
    return (-1);
  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  // Offload the function to the driver and receive the result
  send_offload_message(ocq, mytid, SYSCALL_sys_connect, sockfd, get_pa((QWORD) addr), addrlen, 0, 0, 0);
  iret = (int) receive_offload_message(icq, mytid, SYSCALL_sys_connect);

  return iret;
}

/**
 * @brief Offloading accept systemcall that used with connection-based socket types (SOCK_STREAM, SOCK_SEQPACKET)
 * @param sockfd- a file descriptor referring to the socket
 * @param addr- a pointer to a sockaddr structure
 * @param addrlen- a value-result argument: the caller must initialize it to contain the size (in bytes) of the structure pointed to by addr; on return it will contain the actual size of the peer address
 * @return On success, these system calls return a nonnegative integer that is a descriptor for the accepted socket. On error, -1 is returned, and errno is set appropriately
 */
int sys_off_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  channel_t *ch = NULL;
  struct circular_queue *icq = NULL;
  struct circular_queue *ocq = NULL;

  TCB *current = NULL;
  int mytid = -1;
  int fd = -1;

  // Get the channel for communicating with the driver
  ch = get_offload_channel(-1);
  if(ch == NULL || addr == NULL || addrlen == NULL)
    return (-1);
  icq = ch->in;
  ocq = ch->out;

  current = get_current();
  mytid = current->id;

  // Offload the function to the driver and receive the result
  send_offload_message(ocq, mytid, SYSCALL_sys_accept, sockfd, get_pa((QWORD) addr), get_pa((QWORD) addrlen), 0, 0, 0);
  fd = (int) receive_offload_message(icq, mytid, SYSCALL_sys_accept);

  return fd;
}
