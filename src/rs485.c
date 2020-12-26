/*
 * drv_rs485.c
 *
 * Change Logs:
 * Date           Author            Notes
 * 2020-06-08     qiyongzhong       first version
 * 2020-12-14     qiyongzhong       fix bug and release v1.0
 * 2020-12-17     qiyongzhong       fix log tag
 * 2020-12-18     qiyongzhong       add rs485_send_then_recv
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <rs485.h>

#define DBG_TAG "rs485"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define RS485_EVT_RX_IND    (1<<0)
#define RS485_EVT_RX_BREAK  (1<<1)

struct rs485_inst 
{
    rt_device_t serial;     //serial device handle
    rt_mutex_t lock;        //mutex handle
    rt_event_t evt;         //event handle
    rt_uint8_t status;      //connect status
    rt_uint8_t level;       //control pin send mode level, 0--low, 1--high
    rt_int16_t pin;         //control pin number used, -1--no using
    rt_int32_t timeout;     //receive block timeout, ms   
    rt_int32_t byte_tmo;    //receive byte interval timeout, ms
};

static rt_err_t rs485_recv_ind_hook(rt_device_t dev, rt_size_t size)
{
    rs485_inst_t *hinst = (rs485_inst_t *)(dev->user_data);
    if (hinst->evt)
    {
        rt_event_send(hinst->evt, RS485_EVT_RX_IND);
    }
    return(RT_EOK);
}

static int rs485_cal_byte_tmo(int baudrate)
{
    int tmo = (40 * 1000) / baudrate;
    if (tmo < RS485_BYTE_TMO_MIN)
    {
        tmo = RS485_BYTE_TMO_MIN;
    }
    else if (tmo > RS485_BYTE_TMO_MAX)
    {
        tmo = RS485_BYTE_TMO_MAX;
    }
    return (tmo);
}

static void rs485_mode_set(rs485_inst_t * hinst, int mode)//mode : 0--receive mode, 1--send mode
{
    if (hinst->pin < 0)
    {
        return;
    }
    
    if (mode)
    {
        rt_pin_write(hinst->pin, hinst->level);
    }
    else
    {
        rt_pin_write(hinst->pin, ! hinst->level);
    }
}

/* 
 * @brief   create rs485 instance dynamically
 * @param   serial      - serial device name
 * @param   baudrate    - serial baud rate
 * @param   parity      - serial parity mode
 * @param   pin         - mode contrle pin
 * @param   level       - send mode level
 * @retval  instance handle
 */
rs485_inst_t * rs485_create(const char *name, int baudrate, int parity, int pin, int level)
{
    rs485_inst_t *hinst;
    rt_device_t dev;
    
    dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        LOG_E("rs485 instance initiliaze error, the serial device no found.");
        return(RT_NULL);
    }
    
    if (dev->type != RT_Device_Class_Char)
    {
        LOG_E("rs485 instance initiliaze error, the serial device type is not char.");
        return(RT_NULL);
    }
    
    hinst = rt_malloc(sizeof(struct rs485_inst));
    if (hinst == RT_NULL)
    {
        LOG_E("rs485 create fail. no memory for rs485 create instance.");
        return(RT_NULL);
    }

    hinst->lock = rt_mutex_create(name, RT_IPC_FLAG_FIFO);
    if (hinst->lock == RT_NULL)
    {
        rt_free(hinst);
        LOG_E("rs485 create fail. no memory for rs485 create mutex.");
        return(RT_NULL);
    }

    hinst->evt = rt_event_create(name, RT_IPC_FLAG_FIFO);
    if (hinst->evt == RT_NULL)
    {
        rt_mutex_delete(hinst->lock);
        rt_free(hinst);
        LOG_E("rs485 create fail. no memory for rs485 create event.");
        return(RT_NULL);
    }

    hinst->serial = dev;
    hinst->status = 0;
    hinst->pin = pin;
    hinst->level = (level != 0);
    hinst->timeout = 0;
    hinst->byte_tmo = rs485_cal_byte_tmo(baudrate);
    
    rs485_config(hinst, baudrate, (parity?9:8), parity, 1);

    LOG_D("rs485 create success.");

    return(hinst);
}

/* 
 * @brief   destory rs485 instance created dynamically
 * @param   hinst       - instance handle
 * @retval  0 - success, other - error
 */
int rs485_destory(rs485_inst_t * hinst)
{
    if (hinst == RT_NULL)
    {
        LOG_E("rs485 destory fail. hinst is NULL.");
        return(-RT_ERROR);
    }
    
    rs485_disconn(hinst);

    if (hinst->lock)
    {
        rt_mutex_delete(hinst->lock);
        hinst->lock = RT_NULL;
    }

    if (hinst->evt)
    {
        rt_event_delete(hinst->evt);
        hinst->evt = RT_NULL;
    }
    
    rt_free(hinst);
    
    LOG_D("rs485 destory success.");
    
    return(RT_EOK);
}

/* 
 * @brief   config rs485 params 
 * @param   hinst       - instance handle
 * @param   baudrate    - baudrate of communication
 * @param   databits    - data bits, 5~8
 * @param   parity      - parity bit, 0~2, 0 - none, 1 - odd, 2 - even
 * @param   stopbits    - stop bits, 0~1, 0 - 1 stop bit, 1 - 2 stop bits
 * @retval  0 - success, other - error
 */
int rs485_config(rs485_inst_t * hinst, int baudrate, int databits, int parity, int stopbits)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    if (hinst == RT_NULL)
    {
        LOG_E("rs485 config fail. hinst is NULL.");
        return(-RT_ERROR);
    }

    config.baud_rate = baudrate;
    config.data_bits = databits;
    config.parity = parity;
    config.stop_bits = stopbits;
    rt_device_control(hinst->serial, RT_DEVICE_CTRL_CONFIG, &config);

    return(RT_EOK);
}

/* 
 * @brief   set wait datas timeout for receiving 
 * @param   hinst       - instance handle
 * @param   tmo_ms      - receive wait timeout, 0--no wait, <0--wait forever, >0--wait timeout, default = 0
 * @retval  0 - success, other - error
 */
int rs485_set_recv_tmo(rs485_inst_t * hinst, int tmo_ms)
{
    if (hinst == RT_NULL)
    {
        LOG_E("rs485 set recv timeout fail. hinst is NULL.");
        return(-RT_ERROR);
    }
    
    hinst->timeout = tmo_ms;
    
    LOG_D("rs485 set recv timeout success. the value is %d.", tmo_ms);

    return(RT_EOK);
}
 
/* 
 * @brief   set byte interval timeout for receiving
 * @param   hinst       - instance handle
 * @param   tmo_ms      - byte interval timeout, default is calculated from baudrate
 * @retval  0 - success, other - error
 */
int rs485_set_byte_tmo(rs485_inst_t * hinst, int tmo_ms)
{
    if (hinst == RT_NULL)
    {
        LOG_E("rs485 set byte timeout fail. hinst is NULL.");
        return(-RT_ERROR);
    }
    
    if (tmo_ms < RS485_BYTE_TMO_MIN)
    {
        tmo_ms = RS485_BYTE_TMO_MIN;
    }
    else if (tmo_ms > RS485_BYTE_TMO_MAX)
    {
        tmo_ms = RS485_BYTE_TMO_MAX;
    }
    
    hinst->byte_tmo = tmo_ms;

    LOG_D("rs485 set byte timeout success. the value is %d.", tmo_ms);

    return(RT_EOK);
}

/* 
 * @brief   open rs485 connect
 * @param   hinst       - instance handle
 * @retval  0 - success, other - error
 */
int rs485_connect(rs485_inst_t * hinst)
{
    if (hinst == RT_NULL)
    {
        LOG_E("rs485 connect fail. hinst is NULL.");
        return(-RT_ERROR);
    }

    if (hinst->status == 1)//is connected
    {
        LOG_D("rs485 is connected.");
        return(RT_EOK);
    }
    
    if ( rt_device_open(hinst->serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        LOG_E("rs485 instance connect error. serial open fail.");
        return(-RT_ERROR);
    }
    
    if (hinst->pin >= 0)
    {
        rt_pin_mode(hinst->pin, PIN_MODE_OUTPUT);
        rt_pin_write(hinst->pin, ! hinst->level);
    }


    hinst->serial->user_data = hinst;
    hinst->serial->rx_indicate = rs485_recv_ind_hook;
    hinst->status = 1;

    LOG_D("rs485 connect success.");

    return(RT_EOK);
}

/* 
 * @brief   close rs485 connect
 * @param   hinst       - instance handle
 * @retval  0 - success, other - error
 */
int rs485_disconn(rs485_inst_t * hinst)
{
    if (hinst == RT_NULL)
    {
        LOG_E("rs485 disconnect fail. hinst is NULL.");
        return(-RT_ERROR);
    }
    
    if (hinst->status == 0)//is not connected
    {
        LOG_D("rs485 is not connected.");
        return(RT_EOK);
    }

    rt_mutex_take(hinst->lock, RT_WAITING_FOREVER);

    if (hinst->serial)
    {
        hinst->serial->rx_indicate = RT_NULL;
        rt_device_close(hinst->serial);
    }
    
    if (hinst->pin >= 0)
    {
        rt_pin_mode(hinst->pin, PIN_MODE_INPUT);
    }
    
    hinst->status = 0;
    
    rt_mutex_release(hinst->lock);
    
    LOG_D("rs485 disconnect success.");
    
    return(RT_EOK);
}

/* 
 * @brief   receive datas from rs485
 * @param   hinst       - instance handle
 * @param   buf         - buffer addr
 * @param   size        - maximum length of received datas
 * @retval  >=0 - length of received datas, <0 - error
 */
int rs485_recv(rs485_inst_t * hinst, void *buf, int size)
{
    int recv_len = 0;
    rt_uint32_t recved = 0;
    
    if (hinst == RT_NULL || buf == RT_NULL || size == 0)
    {
        LOG_E("rs485 receive fail. param error.");
        return(-RT_ERROR);
    }
    
    if (hinst->status == 0)
    {
        LOG_E("rs485 receive fail. it is not connected.");
        return(-RT_ERROR);
    }
    
    if (rt_mutex_take(hinst->lock, RT_WAITING_FOREVER) != RT_EOK)
    {
        LOG_E("rs485 receive fail. it is destoried.");
        return(-RT_ERROR);
    }
    
    while(size)
    {
        int len = rt_device_read(hinst->serial, 0, (char *)buf + recv_len, size);
        if (len)
        {
            recv_len += len;
            size -= len;
            continue;
        }
        rt_event_control(hinst->evt, RT_IPC_CMD_RESET, RT_NULL);
        if (recv_len)
        {
            if (rt_event_recv(hinst->evt, RS485_EVT_RX_IND, 
                    (RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR), hinst->byte_tmo, &recved) != RT_EOK)
            {
                break;
            }
        }
        else
        {
            if (rt_event_recv(hinst->evt, (RS485_EVT_RX_IND | RS485_EVT_RX_BREAK), 
                    (RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR), hinst->timeout, &recved) != RT_EOK)
            {
                break;
            }
            if ((recved & RS485_EVT_RX_BREAK) != 0)
            {
                rt_mutex_release(hinst->lock);
                rt_thread_delay(2);
                return(0);
            }
        }
    }
    
    rt_mutex_release(hinst->lock);
    
    return(recv_len);
}

/* 
 * @brief   send datas to rs485
 * @param   hinst       - instance handle
 * @param   buf         - buffer addr
 * @param   size        - length of send datas
 * @retval  >=0 - length of sent datas, <0 - error
 */
int rs485_send(rs485_inst_t * hinst, void *buf, int size)
{
    int send_len = 0;
    
    if (hinst == RT_NULL || buf == RT_NULL || size == 0)
    {
        LOG_E("rs485 send fail. param is error.");
        return(-RT_ERROR);
    }

    if (hinst->status == 0)
    {
        LOG_E("rs485 send fail. it is not connected.");
        return(-RT_ERROR);
    }
    
    if (rt_mutex_take(hinst->lock, RT_WAITING_FOREVER) != RT_EOK)
    {
        LOG_E("rs485 send fail. it is destoried.");
        return(-RT_ERROR);
    }

    rs485_mode_set(hinst, 1);//set to send mode

    send_len = rt_device_write(hinst->serial, 0, buf, size);
    
    rs485_mode_set(hinst, 0);//set to receive mode
    
    rt_mutex_release(hinst->lock);

    return(send_len);
}

/* 
 * @brief   break rs485 receive wait
 * @param   hinst       - instance handle
 * @retval  0 - success, other - error
 */
int rs485_break_recv(rs485_inst_t * hinst)
{
    if ((hinst == RT_NULL) || (hinst->evt == RT_NULL))
    {
        return(-RT_ERROR);
    }

    rt_event_send(hinst->evt, RS485_EVT_RX_BREAK);
    
    return (RT_EOK);
}

/* 
 * @brief   send data to rs485 and then receive response data from rs485
 * @param   hinst       - instance handle
 * @param   send_buf    - send buffer addr
 * @param   send_len    - length of send datas
 * @param   recv_buf    - recv buffer addr
 * @param   recv_size   - maximum length of received datas
 * @retval  >=0 - length of received datas, <0 - error
 */
int rs485_send_then_recv(rs485_inst_t * hinst, void *send_buf, int send_len, void *recv_buf, int recv_size)
{
    int recv_len = 0;
    rt_uint32_t recved = 0;
    
    if (hinst == RT_NULL || send_buf == RT_NULL || send_len == 0 || recv_buf == RT_NULL || recv_size == 0)
    {
        LOG_E("rs485 send then recv fail. param is error.");
        return(-RT_ERROR);
    }

    if (hinst->status == 0)
    {
        LOG_E("rs485 send_then_recv fail. it is not connected.");
        return(-RT_ERROR);
    }

    if (rt_mutex_take(hinst->lock, RT_WAITING_FOREVER) != RT_EOK)
    {
        LOG_E("rs485 send_then_recv fail. it is destoried.");
        return(-RT_ERROR);
    }

    rs485_mode_set(hinst, 1);//set to send mode
    send_len = rt_device_write(hinst->serial, 0, send_buf, send_len);
    rs485_mode_set(hinst, 0);//set to receive mode
    if (send_len < 0)
    {
        rt_mutex_release(hinst->lock);
        LOG_E("rs485 send_then_recv fail. send datas error.");
        return(-RT_ERROR);
    }

    while(recv_size)
    {
        int len = rt_device_read(hinst->serial, 0, (char *)recv_buf + recv_len, recv_size);
        if (len)
        {
            recv_len += len;
            recv_size -= len;
            continue;
        }
        rt_event_control(hinst->evt, RT_IPC_CMD_RESET, RT_NULL);
        if (recv_len)
        {
            if (rt_event_recv(hinst->evt, RS485_EVT_RX_IND, 
                    (RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR), hinst->byte_tmo, &recved) != RT_EOK)
            {
                break;
            }
        }
        else
        {
            if (rt_event_recv(hinst->evt, RS485_EVT_RX_IND, 
                    (RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR), hinst->timeout, &recved) != RT_EOK)
            {
                break;
            }
        }
    }
    
    rt_mutex_release(hinst->lock);
    
    return(recv_len);
}

