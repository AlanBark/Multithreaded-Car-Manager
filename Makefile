all: manager simulator fire_alarm

.PHONY: manager simulator fire_alarm

manager: 
	$(MAKE) -C manager

simulator: 
	$(MAKE) -C simulator

fire_alarm: 
	$(MAKE) -C fire_alarm

.PHONY: clean

clean:
	$(RM) manager.out
	$(RM) simulator.out
	$(RM) fire_alarm.out
	$(MAKE) -C manager clean
	$(MAKE) -C simulator clean
	$(MAKE) -C fire_alarm clean