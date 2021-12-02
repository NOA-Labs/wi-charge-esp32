project description：  
		IDE： Visual Studio + PlatformIO + Arduino.  
		ESP32 Boards： AI Thinker ESP32-CAM.  
		
Command line descriptions:  
		1, input "help" through the serial port assistant, application will printf string following string :  
			help:          Lists all the registered commands.  
			reboot                       : reboot device.  
			ping mcu                     : check connect mcu is ok.  
			set cpu freq=xMHz            : set CPU frequency.  
			get esp32 freq               : get esp32 frequency.  
			task list                    : get all task.  
			deepSleep=msec               : config enter deep sleep mode.  
			connect ap=ssid=password     : config wifi to connect router.  
			disconnect ap                : disconnect router.  
			sync rtc time                : sync rtc time used by network time.  
			download=url                 : url:download path.  
			download test=url            : url:download path.  
			peripheral test=x            : x=SD: sd test.  
		2, connect ap:  
			example: connect ap=NOARDTest=12345678  
		3, test downlod function:
			ex. download test=https://blob.wi-whisper.com/videos/video0_480_272.avi
			

	
	
	