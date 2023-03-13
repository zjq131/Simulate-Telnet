#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define TRUE        1
#define FALSE       -1
#define BUF_SIZE     1024  //传入数据的最大容量1024byte

 

//登录状态标识

char *error_user = "error user name";
char *error_password = "error password";
char *correct_user = "correct user";
char *success_login = "correct password";
char *exit_command = "exit";
char *exit_tip = "退出连接！";

/*
* 介绍：客户端
* 功能：
*    1.登录服务器端
*    2.登录后在服务器端对文件进行操作
*/


int send_recv(int connfd,char flag)
{

   /*
     *  作用：客户端与服务端之间的信息处理
     *  connfd: 服务器端标识
     *  flag: 输入类别标识 【u:用户名  p:密码  c:命令】
    */

    char input_buf[ BUF_SIZE ]; //定义字符串存放输入信息
    char recv_buf[ BUF_SIZE ]; //定义字符串存放服务器端返回信息
    
    memset(input_buf, 0, sizeof(input_buf));// 刷新输入的缓存区
    memset(recv_buf, 0, sizeof(recv_buf));// 刷新接受服务端信息的缓存区

    fgets(input_buf, BUF_SIZE, stdin);//获取用户输入  stdin:标准输入流; input_buf: 保存输入的数据

    input_buf[strlen(input_buf) - 1] = '\0'; //去除最后一个输入的确认[回车]

    printf("客户端正在发送数据...\n");
    /* 向服务器发送消息 */
    if (send(connfd, input_buf, BUF_SIZE, 0) < 0)
        printf("send failed");

    printf("客户端正在接收数据...\n");
    /* 从服务器接受消息 */
    if (recv(connfd, recv_buf, BUF_SIZE, 0) < 0)
        printf("recv failed");

    /* 用户名处理 */
    if (flag == 'u') {
        if (strcmp(recv_buf, error_user) == 0 ) {  //用户名错误
            puts(error_user);
            return FALSE;
        }
    }

    /* 密码处理 */
    else if (flag == 'p') {
        if (strcmp(recv_buf, error_password) == 0)
        {  
            puts(error_password);  //密码错误
            return FALSE;
        }
    }

    /* 命令处理 */
    else if(flag == 'c'){
        if (strcmp(recv_buf, exit_command) == 0) //退出
        {   
            printf("连接已断开\n");          
            return FALSE;
        }
        puts(recv_buf);
    }
    return TRUE;

}

// 主函数
int main( int argc, char *argv[])
{
    int connfd; //服务端标识

    int serv_port;// 服务端端口号

    char recv_buf[BUF_SIZE];// 接受收服务端数据

    struct sockaddr_in  serv_addr;// 保存socket信息

    if (argc != 3) {
        printf("输入格式为: ./c  [ip] [port]\n");
        exit (1);
    }

 
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));//属性置零

    serv_addr.sin_family = AF_INET; // 采取IPv4协议通讯

    serv_addr.sin_port = htons(atoi(argv[2]));// 设置端口号【函数[htons]将主机字节转换成网络字节序】

    inet_aton(argv[1], &serv_addr.sin_addr);// 设置IP地址

    if(serv_addr.sin_port == 0 || serv_addr.sin_addr.s_addr == 0){
        fprintf(stderr, "输入格式为: ./c  [ip] [port]\n");// 打印错误日志
        exit(1);
    }

    connfd = socket(AF_INET, SOCK_STREAM, 0);  //创建socket【服务标识】
    

    if (connfd < 0)
        printf("socket create failed");

    /**
        //连接服务器
        sockfd: socket描述符 ; serv_addr: 包含连接信息的指针 ; addrlen:远端地址·结构·的长度
    */
    if (connect(connfd, (struct sockaddr *)&serv_addr,sizeof(struct sockaddr)) < 0)
    {    
        printf("connect failed");
        exit(0);
    }

    // 连接服务器成功后的操作
    printf("请输入用户名：");

    /*输入命令行操作*/
    if (send_recv(connfd,'u') == TRUE)
    {
        printf("请输入密码：");

        if (send_recv(connfd,'p') == TRUE)
        {
            while(1)
            {
                printf("[zjq@ubantu]:");

                if (send_recv(connfd, 'c') == FALSE)
                {
                    close(connfd);
                    exit(0);
                }
            }
        }

    // 关闭连接
    close(connfd);
    return 0;
    }
}