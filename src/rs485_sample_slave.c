/*
 * rs485_sample_slave.c
 *
 * Change Logs:
 * Date           Author            Notes
 * 2020-12-17     qiyongzhong       first version
 */
    
#include <rtthread.h>
#include <rtdevice.h>
#include <rs485.h>

#define DBG_TAG "rs485.sample.slave"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#ifdef RS485_USING_SAMPLE_SLAVE

#ifndef RS485_SAMPLE_SLAVE_SERIAL
#define RS485_SAMPLE_SLAVE_SERIAL       "uart2"
#endif

#ifndef RS485_SAMPLE_SLAVE_BAUDRATE
#define RS485_SAMPLE_SLAVE_BAUDRATE     9600
#endif

#ifndef RS485_SAMPLE_MASTER_PARITY
#define RS485_SAMPLE_MASTER_PARITY      0 //0 -- none parity
#endif

#ifndef RS485_SAMPLE_SLAVE_PIN
#define RS485_SAMPLE_SLAVE_PIN          -1 //-1 -- nonuse rs485 mode control
#endif

#ifndef RS485_SAMPLE_SLAVE_LVL
#define RS485_SAMPLE_SLAVE_LVL          1
#endif

static void rs485_sample_slave_loopback_test(void *args)
{
    static rt_uint8_t buf[256];
    rs485_inst_t *hinst = rs485_create(RS485_SAMPLE_SLAVE_SERIAL, RS485_SAMPLE_SLAVE_BAUDRATE, 
                                        RS485_SAMPLE_MASTER_PARITY, RS485_SAMPLE_SLAVE_PIN, RS485_SAMPLE_SLAVE_LVL);

    if (hinst == RT_NULL)
    {
        LOG_E("create rs485 instance fail.");
        return;
    }

    rs485_set_recv_tmo(hinst, RT_WAITING_FOREVER);
    if (rs485_connect(hinst) != RT_EOK)
    {
        rs485_destory(hinst);
        LOG_E("rs485 connect fail.");
        return;
    }

    while(1)
    {
        int len = rs485_recv(hinst, buf, sizeof(buf));
        if (len > 0)
        {
            rs485_send(hinst, buf, len);
        }
    }
}

static int rs485_sample_slave_init(void)
{
    rt_thread_t tid = rt_thread_create("rs485slave", rs485_sample_slave_loopback_test, RT_NULL, 1024, 8, 20);
    RT_ASSERT(tid != RT_NULL);
    rt_thread_startup(tid);
    LOG_I("rs485 sample slave thread startup...");
    return(RT_EOK);
}
INIT_APP_EXPORT(rs485_sample_slave_init);

#endif

