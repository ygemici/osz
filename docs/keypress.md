OS/Z How a keypress is processed
================================

   1. PIC or IOAPIC fires IRQ
   2. ISR places an IRQ message in system task's message queue
   3. if not active, task switch to system task
   4. ISR leaves, new IRQs can fire
   5. system thread receives and dispatches message to ps2 driver (shared lib)
   6. ps2 driver handles the hardware (read scancode from keyboard)
   7. sends a message to ui task via syscall
   8. task switch to ui task
   9. ui task receives scancode message
  10. translates scancode to a key press or release
  11. sends a key event message to the focused window's thead via syscall
  12. task switch to the focused window's thread
  13. receives key and draw character
  14. sends a flush message to the ui task
  15. task switch to ui task
  16. composites windows and calls swapbuffer
  17. task switch to system task
  18. system task dispatches message to the lfb driver
  19. the lfb driver copies the buffer to framebuffer
  20. you see what you've typed :-)
