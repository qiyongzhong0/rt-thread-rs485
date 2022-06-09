/*
 * rs485_test.c
 *
 * Change Logs:
 * Date           Author            Notes
 * 2020-06-08     qiyongzhong       first version
 * 2020-12-17     qiyongzhong       add config function
 * 2020-12-18     qiyongzhong       add send_then_recv
 */

#include <rtthread.h>
#include <drv_common.h>
#include <rs485.h>
#include <stdlib.h>
#include <string.h>

#ifdef RS485_USING_TEST

#ifndef RS485_TEST_SERIAL
#define RS485_TEST_SERIAL       "uart1"         //default test serial
#endif

#ifndef RS485_TEST_BAUDRATE
#define RS485_TEST_BAUDRATE     9600            //defalut test baudrate
#endif

#ifndef RS485_TEST_PARITY
#define RS485_TEST_PARITY       0               //defalut test parity
#endif

#ifndef RS485_TEST_PIN
#define RS485_TEST_PIN          GET_PIN(A, 0)   //default test ctrl pin
#endif

#ifndef RS485_TEST_LEVEL
#define RS485_TEST_LEVEL        1               //default test send mode level
#endif

#ifndef RS485_TEST_BUF_SIZE
#define RS485_TEST_BUF_SIZE     1024            //default test buffer size
#endif

#ifndef RS485_TEST_RECV_TMO
#define RS485_TEST_RECV_TMO     30000           //default test recicve timeout
#endif

static rs485_inst_t * test_hinst = RT_NULL;
static char test_buf[RS485_TEST_BUF_SIZE];

static const char *cmd_info[] =
{
    "Usage: \n",
    "rs485 create [serial] [baudrate] [parity] [pin] [level] - create rs485 instance.\n",
    "rs485 destory                                           - destory rs485 instance.\n",
    "rs485 set_recv_tmo [tmo_ms]                             - set recieve timeout.\n",
    "rs485 set_byte_tmo [tmo_ms]                             - set byte timeout.\n",
    "rs485 connect                                           - open rs485 connect.\n",
    "rs485 disconn                                           - close rs485 connect.\n",
    "rs485 recv [size]                                       - receive from rs485.\n",
    "rs485 send [size]                                       - send to rs485.\n",
    "rs485 cfg [baudrate] [databits] [parity] [stopbits]     - config rs485.\n",
    "rs485 send_then_recv [send_size] [recv_size]            - send to rs485 and then receive from rs485.\n",
    "\n"
};

static void show_cmd_info(void)
{
    for(int i=0; i<sizeof(cmd_info)/sizeof(char*); i++)
    {
        rt_kprintf(cmd_info[i]);
    }
}

static void rs485_test(int argc, char **argv)
{
    if (argc < 2)
    {
        show_cmd_info();
        return ;
    }
    
    if (strcmp(argv[1], "create") == 0)
    {
        char *serial = RS485_TEST_SERIAL;
        int baudrate = RS485_TEST_BAUDRATE;
        int parity = RS485_TEST_PARITY;
        int pin = RS485_TEST_PIN;
        int level = RS485_TEST_LEVEL;
        
        if (test_hinst != NULL)
        {
            rt_kprintf("the test instance is not NULL, please deinit/destory first.\n");
            return;
        }
        if (argc >= 3)
        {
            serial = argv[2];
        }
        if ( argc >= 4)
        {
            baudrate = atoi(argv[3]);
        }
        if (argc >= 5)
        {
            parity = atoi(argv[4]);
        }
        if (argc >= 6)
        {
            pin = atoi(argv[5]);
        }
        if (argc >= 7)
        {
            level = atoi(argv[6]);
        }
        test_hinst = rs485_create(serial, baudrate, parity, pin, level);
        if (test_hinst != NULL)
        {
            rt_kprintf("rs485 instance create success.\n");
            rt_kprintf("rs485 serial            : %s \n", serial);
            rt_kprintf("rs485 baudrate          : %d \n", baudrate);
            rt_kprintf("rs485 parity            : %d \n", parity);
            rt_kprintf("rs485 control pin       : %d \n", pin);
            rt_kprintf("rs485 send mode level   : %d \n", level);
            
            rs485_set_recv_tmo(test_hinst, RS485_TEST_RECV_TMO);
            rt_kprintf("rs485 receive timeout   : %d \n", RS485_TEST_RECV_TMO);
        }
        return;
    }
    
    if (strcmp(argv[1], "destory") == 0)
    {
        rs485_destory(test_hinst);
        test_hinst = NULL;
        return;
    }

    if (strcmp(argv[1], "set_recv_tmo") == 0)
    {
        int tmo_ms = RS485_TEST_RECV_TMO;
        if (argc >= 3)
        {
            tmo_ms = atoi(argv[2]);
        }
        rs485_set_recv_tmo(test_hinst, tmo_ms);
        return;
    }
    
    if (strcmp(argv[1], "set_byte_tmo") == 0)
    {
        int tmo_ms = 0;
        if (argc >= 3)
        {
            tmo_ms = atoi(argv[2]);
        }
        rs485_set_byte_tmo(test_hinst, tmo_ms);
        return;
    }
    
    if (strcmp(argv[1], "connect") == 0)
    {
        if (test_hinst == NULL)
        {
            rt_kprintf("the test instance is NULL, please create first.\n");
            return;
        }
        if (rs485_connect(test_hinst) == RT_EOK)
        {
            rt_kprintf("rs485 instance connect success.\n");
        }
        else
        {
            rt_kprintf("rs485 instance connect fail.\n");
        }
        return;
    }
    
    if (strcmp(argv[1], "disconn") == 0)
    {
        rs485_disconn(test_hinst);
        return;
    }
    
    if (strcmp(argv[1], "recv") == 0)
    {
        int size = RS485_TEST_BUF_SIZE;
        int len = 0;
        
        if (test_hinst == NULL)
        {
            rt_kprintf("the test instance is NULL, please create first.\n");
            return;
        }
        if (argc >= 3)
        {
            size = atoi(argv[2]);
            if (size > RS485_TEST_BUF_SIZE)
            {
                size = RS485_TEST_BUF_SIZE;
            }
        }
        rt_kprintf("rs485 start receiving, max length : %d .\n", size);
        len = rs485_recv(test_hinst, test_buf, size);
        if (len == 0)
        {
            rt_kprintf("rs485 receive timeout.\n");
            return;
        }
        rt_kprintf("rs485 received %d datas (hex) : ", len);
        for (int i=0; i<len; i++)
        {
            rt_kprintf("%02X ", test_buf[i]);
        }
        rt_kprintf("\n");
        return;
    }
    
    if (strcmp(argv[1], "send") == 0)
    {
        int size = RS485_TEST_BUF_SIZE;
        
        if (test_hinst == NULL)
        {
            rt_kprintf("the test instance is NULL, please create first.\n");
            return;
        }
        if (argc >= 3)
        {
            size = atoi(argv[2]);
            if (size > RS485_TEST_BUF_SIZE)
            {
                size = RS485_TEST_BUF_SIZE;
            }
        }
        for (int i=0; i<size; i++)
        {
            test_buf[i] = i;
        }
        size = rs485_send(test_hinst, test_buf, size);
        rt_kprintf("rs485 transmit completed. length : %d .\n", size);
        return;
    }
    
    if (strcmp(argv[1], "cfg") == 0)
    {
        int baudrate = RS485_TEST_BAUDRATE;
        int databits = 8;
        int parity = RS485_TEST_PARITY;
        int stopbits = 1;
        
        if (test_hinst == NULL)
        {
            rt_kprintf("the test instance is NULL, please create first.\n");
            return;
        }
        if (argc >= 3)
        {
            baudrate = atoi(argv[2]);
        }
        if ( argc >= 4)
        {
            databits = atoi(argv[3]);
        }
        if (argc >= 5)
        {
            parity = atoi(argv[4]);
        }
        if (argc >= 6)
        {
            stopbits = atoi(argv[5]);
        }
        rs485_config(test_hinst, baudrate, databits, parity, stopbits);
        return;
    }
    
    if (strcmp(argv[1], "send_then_recv") == 0)
    {
        int send_size = RS485_TEST_BUF_SIZE;
        int recv_size = RS485_TEST_BUF_SIZE;
        int len = 0;
        
        if (test_hinst == NULL)
        {
            rt_kprintf("the test instance is NULL, please create first.\n");
            return;
        }
        if (argc >= 3)
        {
            send_size = atoi(argv[2]);
        }
        if (argc >= 4)
        {
            recv_size = atoi(argv[3]);
        }
        for (int i=0; i<send_size; i++)
        {
            test_buf[i] = i;
        }
        rt_kprintf("rs485 send %d datas, then receive max length : %d .\n", send_size, recv_size);
        len = rs485_send_then_recv(test_hinst, test_buf, send_size, test_buf, recv_size);
        if (len == 0)
        {
            rt_kprintf("rs485 receive timeout.\n");
            return;
        }
        rt_kprintf("rs485 received %d datas (hex) : ", len);
        for (int i=0; i<len; i++)
        {
            rt_kprintf("%02X ", test_buf[i]);
        }
        rt_kprintf("\n");
        return;
    }
    
    rt_kprintf("error ! unsupported command .\n");
}
MSH_CMD_EXPORT_ALIAS(rs485_test, rs485, test rs485 module functions);
#endif
