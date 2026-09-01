package main

import "testing"

func TestParseDisassembly(t *testing.T) {
	disassembly := []byte(`
00000000 <App::app_decode()>:
   0: 0b 12        push r11
   2: 0a 12        push r10
   4: 31 50 de ff  add #-34, r1
   8: 31 50 22 00  add #34, r1
   c: 3a 41        pop r10
   e: 3b 41        pop r11

00000010 <helper()>:
  10: 21 83        decd r1
  12: 21 53        incd r1
`)

	frames, err := parseDisassembly("test.o", disassembly)
	if err != nil {
		t.Fatal(err)
	}
	if len(frames) != 2 {
		t.Fatalf("got %d frames, want 2", len(frames))
	}
	if got := frames[0].FrameBytes; got != 38 {
		t.Errorf("app frame = %d, want 38", got)
	}
	if got := frames[1].FrameBytes; got != 2 {
		t.Errorf("helper frame = %d, want 2", got)
	}
}

func TestSelectFrameRejectsAmbiguousSelector(t *testing.T) {
	frames := []functionFrame{
		{Name: "Widget::write(int)"},
		{Name: "Widget::write(char)"},
	}
	if _, err := selectFrame(frames, "Widget::write"); err == nil {
		t.Fatal("expected ambiguous selector error")
	}
}

func TestSelectFrameAcceptsIdenticalCopies(t *testing.T) {
	frames := []functionFrame{
		{Name: "Widget::write(int)", Input: "object.o", FrameBytes: 6},
		{Name: "Widget::write(int)", Input: "firmware.elf", FrameBytes: 6},
	}
	frame, err := selectFrame(frames, "Widget::write")
	if err != nil {
		t.Fatal(err)
	}
	if frame.FrameBytes != 6 {
		t.Fatalf("frame = %d, want 6", frame.FrameBytes)
	}
}
