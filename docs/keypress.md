OS/Z How a keypress is processed
================================

  1. PIC or IOAPIC fires IRQ
  2. ISR places an IRQ message in system process' message queue
  3. if not active, task switch to system process
  4. ISR leaves, new IRQs can fire
  5. system thread receives and dispatches message to ps2 driver (shared lib)
  6. ps2 driver handles the hardware (read scancode from keyboard)
  7. sends a message to ui process via syscall
  8. task switch to ui process
  9. ui process receives scancode message
 10. translates scancode to key
 11. sends a key event message to the focused window's process via syscall
 12. task switch to the focused window's process
 13. receives key and draw character
 14. sends a flush message to the ui process
 15. task switch to ui process
 16. composites windows and swaps buffer
 17. sends a message to system process to flush buffer to video
 18. task switch to system process
 19. system process dispatches message to the lfb driver
 20. the lfb driver copies the buffer to framebuffer
 21. you see what you've typed :-)
