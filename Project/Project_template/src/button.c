#include "stm8l15x.h"
#include "stm8l15x_gpio.h"
#include "stm8l15x_exti.h"

#include "timer.h"
#include "button.h"

#define LEDS_PORT    (GPIOB)
#define LED_PIN1     (GPIO_Pin_0)

#define BUTTON_PORT  (GPIOB)
#define BUTTON_PIN1  (GPIO_Pin_6)
#define BUTTON_PIN2  (GPIO_Pin_7)

#define BUTTON_DEBONCE_DURATION    3    // The unit is 10 ms, so the duration is 30 ms.
#define BUTTON_WAIT_2S             200  // The unit is 10 ms, so the duration is 2 s.
#define BUTTON_WAIT_3S             300  // The unit is 10 ms, so the duration is 3 s.
#define BUTTON_DOUBLE_BTN_DURATION 50   // The unit is 10 ms, so the duration is 500 ms.
#define BUTTON_DOUBLE_BTN_TRACK_DURATION 300 // The unit is 10 ms, so the duration is 3 s.

typedef enum button_timer_status_e
{
	BUTTON_STATUS_INIT = 0,
	BUTTON_STATUS_LESS_2S,
	BUTTON_STATUS_MORE_2S,
	BUTTON_STATUS_MORE_5S,
	BUTTON_STATUS_DOUBLE_TRACK
}button_timer_status_t;

typedef enum button_event_e
{
	BUTTON_INVALID = 0,
	BUTTON1_SHORT_PRESS,
	BUTTON1_DOUBLE_PRESS,
	BUTTON1_LONG_HOLD,
	BUTTON1_LONG_PRESS,
	BUTTON1_VERY_LONG_HOLD,
	BUTTON1_VERY_LONG_PRESS,
	BUTTON2_SHORT_PRESS,
	BUTTON2_DOUBLE_PRESS,
	BUTTON2_LONG_HOLD,
	BUTTON2_LONG_PRESS,
	BUTTON2_VERY_LONG_HOLD,
	BUTTON2_VERY_LONG_PRESS,
	DOUBLE_BTN_TRACK
} button_event_t;

static button_timer_status_t  m_button1_timer_status = BUTTON_STATUS_INIT;
static button_timer_status_t  m_button2_timer_status = BUTTON_STATUS_INIT;

static bool detect_double_button1_press = FALSE;
static bool detect_double_button2_press = FALSE;

static bool button1_is_pushed = FALSE;
static bool button2_is_pushed = FALSE;
static bool double_button_track = FALSE;

static u8   button_status = 0xFF;
static u8   button_first_detect_status = 0xFF;

static u8   m_timer_id_button1_detet;
static u8   m_timer_id_double_btn1_detet;
static u8   m_timer_id_button2_detet;
static u8   m_timer_id_double_btn2_detet;

static u8   m_timer_id_debonce_detet;

extern u32 int_timer1;
extern u32 int_timer2;

void app_button_event_handler(button_event_t button_event);
void button1_push(void);
void button1_release(void);
void button2_push(void);
void button2_release(void);

void Delay(u16 nCount)
{
  /* Decrement nCount value */
  while (nCount != 0)
  {
    nCount--;
  }
}

static void button1_duration_timeout_handler(void)
{
	button_event_t button_event = BUTTON_INVALID;
	switch (m_button1_timer_status)
	{
		case BUTTON_STATUS_INIT:
		{
			break;
		}
		case BUTTON_STATUS_LESS_2S:
		{
			button_event = BUTTON1_LONG_HOLD;
			timer_start(m_timer_id_button1_detet, BUTTON_WAIT_3S);
			m_button1_timer_status = BUTTON_STATUS_MORE_2S;
			break;
		}
		case BUTTON_STATUS_MORE_2S:
		{
			button_event = BUTTON1_VERY_LONG_HOLD;
			m_button1_timer_status = BUTTON_STATUS_MORE_5S;
			break;
		}
		case BUTTON_STATUS_MORE_5S:
		{
			break;
		}
		case BUTTON_STATUS_DOUBLE_TRACK:
		{
			button_event = DOUBLE_BTN_TRACK;
			m_button1_timer_status = BUTTON_STATUS_INIT;
		}
		default:
		{
			break;
		}
	}
	if (button_event != BUTTON_INVALID)
	{
		app_button_event_handler(button_event);
	}
}

static void button2_duration_timeout_handler(void)
{
	button_event_t button_event = BUTTON_INVALID;
	switch (m_button2_timer_status)
	{
		case BUTTON_STATUS_INIT:
		{
			break;
		}
		case BUTTON_STATUS_LESS_2S:
		{
			button_event = BUTTON2_LONG_HOLD;
			timer_start(m_timer_id_button2_detet, BUTTON_WAIT_3S);
			m_button2_timer_status = BUTTON_STATUS_MORE_2S;
			break;
		}
		case BUTTON_STATUS_MORE_2S:
		{
			button_event = BUTTON2_VERY_LONG_HOLD;
			m_button2_timer_status = BUTTON_STATUS_MORE_5S;
			break;
		}
		case BUTTON_STATUS_MORE_5S:
		{
			break;
		}
		case BUTTON_STATUS_DOUBLE_TRACK:
		{
			break;
		}
		default:
		{
			break;
		}
	}
	if (button_event != BUTTON_INVALID)
	{
		app_button_event_handler(button_event);
	}
}

void double_btn1_timeout_handler(void)
{
	button_event_t button_event = BUTTON1_SHORT_PRESS;
	detect_double_button1_press = FALSE;
	m_button1_timer_status = BUTTON_STATUS_INIT;
	timer_stop(m_timer_id_double_btn1_detet);
	app_button_event_handler(button_event);
}

void double_btn2_timeout_handler(void)
{
	button_event_t button_event = BUTTON2_SHORT_PRESS;
	detect_double_button2_press = FALSE;
	m_button2_timer_status = BUTTON_STATUS_INIT;
	timer_stop(m_timer_id_double_btn2_detet);
	app_button_event_handler(button_event);
}

void btn_debonce_timeout_handler(void)
{
	u8 valid_button;
	u8 current_button;
	u8 changed_button;

	current_button = GPIO_ReadInputData(BUTTON_PORT);


	valid_button = ~(current_button ^ button_first_detect_status);


	changed_button = ((current_button^button_status) & valid_button);
	button_status = current_button;
	if ((changed_button & BUTTON_PIN1)!= 0)
	{
		timer_stop(m_timer_id_button1_detet);
		if ((current_button & BUTTON_PIN1) == 0)
		{
			button1_push();
		}
		else
		{
			button1_release();
		}
	}

	if ((changed_button & BUTTON_PIN2)!= 0)
	{
		timer_stop(m_timer_id_button2_detet);
		if ((current_button & BUTTON_PIN2) == 0)
		{
			button2_push();
		}
		else
		{
			button2_release();
		}
	}
}


void button_init()
{
  disableInterrupts();
  GPIO_Init(LEDS_PORT, (LED_PIN1), GPIO_Mode_Out_PP_Low_Fast);
  GPIO_Init(BUTTON_PORT, (BUTTON_PIN1 | BUTTON_PIN2), GPIO_Mode_In_PU_IT);
  EXTI_DeInit();
  EXTI_SelectPort(EXTI_Port_B);
  EXTI_SetPinSensitivity(EXTI_Pin_6, EXTI_Trigger_Rising_Falling);
  EXTI_SetPinSensitivity(EXTI_Pin_7, EXTI_Trigger_Rising_Falling);
  enableInterrupts();

  timer_create(&m_timer_id_button1_detet, button1_duration_timeout_handler);
  timer_create(&m_timer_id_double_btn1_detet, double_btn1_timeout_handler);
  timer_create(&m_timer_id_button2_detet, button2_duration_timeout_handler);
  timer_create(&m_timer_id_double_btn2_detet, double_btn2_timeout_handler);
  timer_create(&m_timer_id_debonce_detet, btn_debonce_timeout_handler);
}


void btn_short_button1_press(void)
{
        GPIO_WriteBit(LEDS_PORT, LED_PIN1, SET);
		Delay(10000);
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, RESET);
}

void btn_double_button1_press(void)
{
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, SET);
		Delay(20000);
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, RESET);
		Delay(20000);
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, SET);
		Delay(20000);
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, RESET);
}

void btn_long_hold_button1_press(void)
{
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, SET);
		Delay(10000);
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, RESET);
}

void btn_long_button1_press(void)
{
}

void btn_very_long_hold_button1_press(void)
{
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, SET);
		Delay(10000);
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, RESET);
}

void btn_very_long_button1_press(void)
{
}

void btn_short_button2_press(void)
{
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, SET);
		Delay(10000);
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, RESET);
}

void btn_double_button2_press(void)
{
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, SET);
		Delay(20000);
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, RESET);
		Delay(20000);
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, SET);
		Delay(20000);
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, RESET);
}

void btn_long_hold_button2_press(void)
{
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, SET);
		Delay(10000);
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, RESET);
}

void btn_long_button2_press(void)
{
}

void btn_very_long_hold_button2_press(void)
{
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, SET);
		Delay(10000);
		GPIO_WriteBit(LEDS_PORT, LED_PIN1, RESET);
}

void btn_very_long_button2_press(void)
{
}

void btn_double_long_hold_press(void)
{
	GPIO_WriteBit(LEDS_PORT, LED_PIN1, SET);
	Delay(30000);
	GPIO_WriteBit(LEDS_PORT, LED_PIN1, RESET);
	Delay(30000);
	GPIO_WriteBit(LEDS_PORT, LED_PIN1, SET);
	Delay(30000);
	GPIO_WriteBit(LEDS_PORT, LED_PIN1, RESET);
}

void app_button_event_handler(button_event_t button_event)
{
	switch (button_event)
	{
		case BUTTON_INVALID:
		{
			break;
		}
		case BUTTON1_SHORT_PRESS:
		{
			btn_short_button1_press();
			break;
		}
		case BUTTON1_DOUBLE_PRESS:
		{
			btn_double_button1_press();
			break;
		}
		case BUTTON1_LONG_HOLD:
		{
			btn_long_hold_button1_press();
			break;
		}
		case BUTTON1_LONG_PRESS:
		{
			btn_long_button1_press();
			break;
		}
		case BUTTON1_VERY_LONG_HOLD:
		{
			btn_very_long_hold_button1_press();
			break;
		}
		case BUTTON1_VERY_LONG_PRESS:
		{
			btn_very_long_button1_press();
			break;
		}
		case BUTTON2_SHORT_PRESS:
		{
			btn_short_button2_press();
			break;
		}
		case BUTTON2_DOUBLE_PRESS:
		{
			btn_double_button2_press();
			break;
		}
		case BUTTON2_LONG_HOLD:
		{
			btn_long_hold_button2_press();
			break;
		}
		case BUTTON2_LONG_PRESS:
		{
			btn_long_button2_press();
			break;
		}
		case BUTTON2_VERY_LONG_HOLD:
		{
			btn_very_long_hold_button2_press();
			break;
		}
		case BUTTON2_VERY_LONG_PRESS:
		{
			btn_very_long_button2_press();
			break;
		}
		case DOUBLE_BTN_TRACK:
		{
			btn_double_long_hold_press();
		}
		default:
		{
			break;
		}
	}
}

// Only use button1_timer to track double button long hold.
void check_track_double_button(void)
{
	if ((button1_is_pushed == TRUE) && (button2_is_pushed == TRUE))
	{
		timer_stop(m_timer_id_button2_detet);  // Disable btn2_timer when tracking double hold.
		m_button1_timer_status = BUTTON_STATUS_DOUBLE_TRACK;
		m_button2_timer_status = BUTTON_STATUS_INIT;
		double_button_track = TRUE;
		timer_start(m_timer_id_button2_detet,BUTTON_DOUBLE_BTN_TRACK_DURATION);  //3 s
	}
	else
	{
		if (double_button_track == TRUE)
		{
			m_button1_timer_status = BUTTON_STATUS_INIT;
			m_button2_timer_status = BUTTON_STATUS_INIT;
			double_button_track = FALSE;
			timer_stop(m_timer_id_button2_detet);
		}
	}
}

void button1_push(void)
{
	button1_is_pushed = TRUE;

	check_track_double_button();

	if (double_button_track == FALSE)
	{
		m_button1_timer_status = BUTTON_STATUS_LESS_2S;
		timer_start(m_timer_id_button1_detet, BUTTON_WAIT_2S);
	}
}

void button1_release(void)
{
	button1_is_pushed = FALSE;
	button_event_t button_event = BUTTON_INVALID;

	check_track_double_button();

	switch (m_button1_timer_status)
	{
		case BUTTON_STATUS_INIT:
			{
				break;
			}
		case BUTTON_STATUS_LESS_2S:
			{
				if (detect_double_button1_press == FALSE)
				{
					detect_double_button1_press = TRUE;
					timer_start(m_timer_id_double_btn1_detet,BUTTON_DOUBLE_BTN_DURATION);  //500ms
				}
				else
				{
					button_event = BUTTON1_DOUBLE_PRESS;
					detect_double_button1_press = FALSE;
					timer_stop(m_timer_id_double_btn1_detet);
				}
				break;
			}
		case BUTTON_STATUS_MORE_2S:
			{
				button_event = BUTTON1_LONG_PRESS;
				break;
			}
		case BUTTON_STATUS_MORE_5S:
			{
				button_event = BUTTON1_VERY_LONG_PRESS;
				break;
			}
		default:
			{
				break;
			}
	}
	m_button1_timer_status = BUTTON_STATUS_INIT;
	if (button_event != BUTTON_INVALID)
	{
		app_button_event_handler(button_event);
	}
}

void button2_push(void)
{
	button2_is_pushed = TRUE;

	check_track_double_button();

	if (double_button_track == FALSE)
	{
		m_button2_timer_status = BUTTON_STATUS_LESS_2S;
		timer_start(m_timer_id_button2_detet, BUTTON_WAIT_2S);
	}
}

void button2_release(void)
{
	button2_is_pushed = FALSE;
	button_event_t button_event = BUTTON_INVALID;

	check_track_double_button();

	switch (m_button2_timer_status)
	{
		case BUTTON_STATUS_INIT:
			{
				break;
			}
		case BUTTON_STATUS_LESS_2S:
			{
				if (detect_double_button2_press == FALSE)
				{
					detect_double_button2_press = TRUE;
					timer_start(m_timer_id_double_btn2_detet,BUTTON_DOUBLE_BTN_DURATION);  //500ms
				}
				else
				{
					button_event = BUTTON2_DOUBLE_PRESS;
					detect_double_button2_press = FALSE;
					timer_stop(m_timer_id_double_btn2_detet);
				}
				break;
			}
		case BUTTON_STATUS_MORE_2S:
			{
				button_event = BUTTON2_LONG_PRESS;
				break;
			}
		case BUTTON_STATUS_MORE_5S:
			{
				button_event = BUTTON2_VERY_LONG_PRESS;
				break;
			}
		default:
			{
				break;
			}
	}
	m_button2_timer_status = BUTTON_STATUS_INIT;
	if (button_event != BUTTON_INVALID)
	{
		app_button_event_handler(button_event);
	}
}

void button_event_handler(void)
{
	button_first_detect_status = GPIO_ReadInputData(BUTTON_PORT);
	timer_start(m_timer_id_debonce_detet, BUTTON_DEBONCE_DURATION);
}
