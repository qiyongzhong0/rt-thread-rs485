/*
 * rs485_sample_master.c
 *
 * Change Logs:
 * Date           Author            Notes
 * 2020-12-17     qiyongzhong       first version
 * 2020-12-18     qiyongzhong       fix to rs485_send_then_recv
 */
    
#include <rtthread.h>
#include <rtdevice.h>
#include <drv_common.h>
#include <rs485.h>
#include <string.h>

#define DBG_TAG "rs485.sample.master"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#ifdef RS485_USING_SAMPLE_MASTER

#ifndef RS485_SAMPLE_MASTER_SERIAL
#define RS485_SAMPLE_MASTER_SERIAL      "uart2" //serial device name
#endif

#ifndef RS485_SAMPLE_MASTER_BAUDRATE
#define RS485_SAMPLE_MASTER_BAUDRATE    9600
#endif

#ifndef RS485_SAMPLE_MASTER_PARITY
#define RS485_SAMPLE_MASTER_PARITY      0 //0 -- none parity
#endif

#ifndef RS485_SAMPLE_MASTER_PIN
#define RS485_SAMPLE_MASTER_PIN         -1 //-1 -- nonuse rs485 mode control
#endif

#ifndef RS485_SAMPLE_MASTER_LVL
#define RS485_SAMPLE_MASTER_LVL         1
#endif

static void rs485_sample_master(void *args)
{
    const char read_cmd[] = "read datas test\r\n";
    static rt_uint8_t buf[256];
    rs485_inst_t *hinst = rs485_create(RS485_SAMPLE_MASTER_SERIAL, RS485_SAMPLE_MASTER_BAUDRATE,
                                        RS485_SAMPLE_MASTER_PARITY, RS485_SAMPLE_MASTER_PIN, RS485_SAMPLE_MASTER_LVL);

    if (hinst == RT_NULL)
    {
        LOG_E("create rs485 instance fail.");
        return;
    }

    rs485_set_recv_tmo(hinst, 1000);
    if (rs485_connect(hinst) != RT_EOK)
    {
        rs485_destory(hinst);
        LOG_E("rs485 connect fail.");
        return;
    }

    while(1)
    {
        int len = strlen(read_cmd);
        len = rs485_send_then_recv(hinst, (void *)read_cmd, len, buf, sizeof(buf));
        if (len < 0)
        {
            LOG_E("rs485 send datas error.");
            break;
        }
        
        if (len == 0)
        {
            LOG_D("rs485 recv timeout.");
            continue;
        }
        
        buf[len] = 0;
        LOG_D("rs485 recv %d datas : %s", len, buf);
    }
    
    rs485_destory(hinst);
    LOG_D("rs485 test end.");
}

static int rs485_sample_master_init(void)
{
    rt_thread_t tid = rt_thread_create("rs485master", rs485_sample_master, RT_NULL, 1024, 8, 20);
    RT_ASSERT(tid != RT_NULL);
    rt_thread_startup(tid);
    LOG_I("rs485 sample master thread startup...");
    return(RT_EOK);
}
INIT_APP_EXPORT(rs485_sample_master_init);

#endif

