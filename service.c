#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT  3333

//用户名和密码

char *name="zjq";

char *passwd="123456";

#define MAX_LISTEN    10
#define BUF_SIZE     1024
#define USERNAME   0
#define PASSWORD   1   
#define COMMAND    2


//登录过程状态标识

char *error_user = "error user name";

char *error_password = "error password";

char *correct_user = "correct user";

char *success_login = "correct password";

char *exit_command = "exit";

char *exit_tip = "退出连接！";

char *wday[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};// 日期枚举

/*
* 介绍：服务器端
* 功能：
*    1.接受客户端的登录请求并验证
*    2.接受客户端的命令在本地执行并返回执行结果
*/


int command(int connfd,char*command)
{    
    /*
     *作用：执行command命令
     *connfd: 客户端标识
     *command: 命令字符串
    */

    FILE *fstream = NULL;      

    char buff[1024];  //保存命令执行结果

    memset(buff, 0, sizeof(buff));//置空 

    //退出
    if(strcmp(exit_command, command) == 0 ) {
        if(send(connfd, exit_command,strlen(exit_command), 0) < 0 )
            printf("send failed");
        printf("客户端已断开连接！\n");
        return -2;
    }
    // 在服务器端执行命令
    if(NULL == (fstream = popen(command,"r")))      
    {   
        // 执行出错
        fprintf(stderr,"execute command failed: %s",strerror(errno));      
        return -1;
    }
    // 防止操作结果为空,无法继续通信 【例如 chmod】
    if(NULL == fgets(buff, sizeof(buff), fstream)){
        char end_buff[1];
        end_buff[0] = 0;
        // 防止操作返回值为空
        if(send(connfd,end_buff,1,0) < 0){//将执行结果返回给至客户端
            printf("send failed");    
        }
    }

    // 发送第一次读取的数据
    printf("%d 读取的字符长度\n",strlen(buff));
    if(send(connfd, buff,strlen(buff), 0) < 0){//将执行结果返回给至客户端
        printf("send failed");    
    }
    // 循环读取命令的结果,并输出到客户端
    while(NULL != fgets(buff, sizeof(buff), fstream))
    {  
        if(send(connfd, buff,strlen(buff), 0) < 0){//将执行结果返回给至客户端
            printf("send failed");    
        }
    }
    // 关闭资源
    pclose(fstream);
    return 0;     
}

 
int main(int argc, char *argv[])
{

    int sockfd, connfd;// 服务端标识，客户端的连接标识

    int optval;

    int recv_type = USERNAME;  //默认为 USERNAME，即用户名输入模式

    int client_size;// 客户端地址结构大小

    int recv_bits; // 记录客户端的传入流的大小

    char buffer[BUF_SIZE];// 存储客户端传入的信息

    FILE *log; // 输出日志指针

    time_t timep;// 输出时间

    struct tm *p;// 日期操作指针

    pid_t pid;// 进程Id

    struct sockaddr_in client_addr, server_addr;// socket结构体 【客户端,服务端】

    client_size = sizeof(struct sockaddr_in);

    /* 初始化各变量为零 */

    memset(buffer, 0, BUF_SIZE);
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(server_addr));


    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)  //创建socket 【服务端标识】
        printf("socket failed");

    /* 设置端口快速重用*/
    optval = 1;
    // int setsockopt(int s, int level, int optname, const void * optval, ,socklen_toptlen);
    // SOL_SOCKET: 存取socket层; optname: SO_REUSEADDR允许在bind()过程中本地地址可重复使用
    // optval:代表欲设置的值 ; optlen: optval 的长度.
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(int)) < 0)
        printf( "setsockopt failed" );

    server_addr.sin_family = AF_INET;

    server_addr.sin_port = htons(PORT);

    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 设置本机地址

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) < 0) //绑定ip和端口号
        printf("bind failed");

    if (listen(sockfd, MAX_LISTEN) < 0) //建立监听
        printf("listen failed");

    printf("Telent服务器已开启......\n");
    // 服务器建立成功...
    while (1) {
        if((connfd = accept(sockfd,(struct sockaddr*)&client_addr, &client_size)) < 0 ) //响应客户端请求
        {
            printf("accept failed");
            close(sockfd);
            close(connfd);
            break;
        }
        printf("服务端接受客户端的地址: ip = %s\n",inet_ntoa(client_addr.sin_addr)); //打印客户端ip
        /* 通过父进程创建子进程处理客户端请求 */
        if ((pid = fork()) == 0){  // 返回值为 0,表示此进程为子进程 【该函数连续返回两次结果】
            while (1) {
                // 接收客户端数据
                if ((recv_bits = recv(connfd, buffer, BUF_SIZE, 0)) < 0 ){
                    printf("recv failed");
                    break;
                }
                /* 去除最后一个字符!!! */
                buffer[recv_bits - 1] = '\0';
                /* 用户名鉴定 */
                if(recv_type == USERNAME){
                    if(strcmp(name, buffer) != 0 ) {
                        if(send(connfd, error_user,strlen(error_user),0) < 0){
                            printf("send failed");
                            break;
                        } 
                    }else{
                        if(send(connfd, correct_user,strlen(correct_user), 0) < 0 )
                        {
                            printf("send failed");
                            break;
                        }
                    }
                    printf("%s正在登录...",buffer);
                    recv_type = PASSWORD;  //设置标志为密码模式
                }
                /* 确认密码 */
                else if(recv_type == PASSWORD){
                    if(strcmp(passwd, buffer) != 0) { //验证密码
                        if(send(connfd, error_password,strlen(error_password), 0) < 0 )
                           {
                            printf("send failed");
                            break;
                           }
                    }else{
                       if(send(connfd, success_login, strlen(success_login), 0) < 0)
                       {
                            printf("send failed");
                            break;
                       }
                       printf("用户登录成功！\n");
                       recv_type = COMMAND;  //将登录标志切换命令行模式模拟远程连接
                    }                
                }
               /* 命令模式 */
                else if(recv_type == COMMAND ) {
                    if(command(connfd,buffer) < 0) break;
                    puts("客户端指令: ");
                    puts(buffer);
                    printf("打印日志...\n");
                    // 输出操作日志
                    if((log = fopen("operate.log","a+")) == NULL){
                        printf("can not open file...\n");
                        break;
                    }
                    // 取得当前时间
                    time (&timep);
                    p = localtime(&timep);
                    printf("%s---\n",asctime(gmtime(&timep)));
                    fprintf(log,"当前用户: %s 操作:%s  操作时间:  %d年 %d月 %d日  %s  %d:%d:%d \n",
                    name,buffer,(1900+p->tm_year),(1+p->tm_mon),p->tm_mday,
                    wday[p->tm_wday],p->tm_hour,p->tm_min,p->tm_sec);
                    fclose(log);
                }       
            }
            // 关闭通道 【资源】
            printf("\n资源已关闭...");
            close(sockfd);
            close(connfd);
            exit(0);
        }
        else{
            close(connfd);
        }
    }
    return 0;
}