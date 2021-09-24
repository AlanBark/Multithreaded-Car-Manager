all: manager simulator fire_alarm

.PHONY: manager simulator fire_alarm

manager: 
	$(MAKE) -C manager

simulator: 
	$(MAKE) -C simulator

fire_alarm: 
	$(MAKE) -C fire_alarm