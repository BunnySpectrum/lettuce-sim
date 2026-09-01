// Command stackusage measures MSP430 function frames from objdump output and
// adds them along a user-supplied call path.
package main

import (
	"bufio"
	"bytes"
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"sort"
	"strconv"
	"strings"
	"text/tabwriter"
)

const msp430ReturnAddressSize = 2

type stringList []string

func (values *stringList) String() string { return strings.Join(*values, ",") }
func (values *stringList) Set(value string) error {
	*values = append(*values, value)
	return nil
}

type functionFrame struct {
	Name       string
	Input      string
	FrameBytes int
}

var (
	functionHeader  = regexp.MustCompile(`^[[:xdigit:]]+ <(.+)>:$`)
	pushInstruction = regexp.MustCompile(`\bpush(?:\.[[:alpha:]])?\b`)
	popInstruction  = regexp.MustCompile(`\bpop(?:\.[[:alpha:]])?\b`)
	addSP           = regexp.MustCompile(`\badd\s+#(-?[0-9]+),\s*r1\b`)
	subSP           = regexp.MustCompile(`\bsub\s+#([0-9]+),\s*r1\b`)
	decdSP          = regexp.MustCompile(`\bdecd\s+r1\b`)
	incdSP          = regexp.MustCompile(`\bincd\s+r1\b`)
	decSP           = regexp.MustCompile(`\bdec\s+r1\b`)
	incSP           = regexp.MustCompile(`\binc\s+r1\b`)
)

func main() {
	var inputs stringList
	var selectors stringList
	flag.Var(&inputs, "input", "object or ELF file to disassemble; repeat as needed")
	flag.Var(&selectors, "function", "function-name substring in call-path order; repeat as needed")
	objdump := flag.String("objdump", defaultObjdump(), "MSP430 objdump executable")
	flag.Parse()

	if flag.NArg() != 0 {
		exitf("unexpected arguments: %s", strings.Join(flag.Args(), " "))
	}
	if len(inputs) == 0 {
		exitf("at least one -input is required")
	}
	if len(selectors) == 0 {
		exitf("at least one -function is required")
	}

	var frames []functionFrame
	for _, input := range inputs {
		parsed, err := disassemble(*objdump, input)
		if err != nil {
			exitf("inspect %s: %v", input, err)
		}
		frames = append(frames, parsed...)
	}

	path := make([]functionFrame, 0, len(selectors))
	for _, selector := range selectors {
		frame, err := selectFrame(frames, selector)
		if err != nil {
			exitf("select %q: %v", selector, err)
		}
		path = append(path, frame)
	}

	render(path)
}

func defaultObjdump() string {
	home, err := os.UserHomeDir()
	if err == nil {
		candidate := filepath.Join(home, ".platformio", "packages", "toolchain-timsp430", "bin", "msp430-objdump")
		if info, statErr := os.Stat(candidate); statErr == nil && !info.IsDir() {
			return candidate
		}
	}
	return "msp430-objdump"
}

func disassemble(objdump, input string) ([]functionFrame, error) {
	command := exec.Command(objdump, "-drC", input)
	output, err := command.CombinedOutput()
	if err != nil {
		return nil, fmt.Errorf("%s -drC: %w\n%s", objdump, err, output)
	}
	return parseDisassembly(input, output)
}

func parseDisassembly(input string, output []byte) ([]functionFrame, error) {
	type frameState struct {
		name    string
		current int
		maximum int
	}

	var result []functionFrame
	var state *frameState
	flush := func() {
		if state == nil {
			return
		}
		result = append(result, functionFrame{
			Name:       state.name,
			Input:      input,
			FrameBytes: state.maximum,
		})
	}

	scanner := bufio.NewScanner(bytes.NewReader(output))
	for scanner.Scan() {
		line := scanner.Text()
		if match := functionHeader.FindStringSubmatch(line); match != nil {
			flush()
			state = &frameState{name: match[1]}
			continue
		}
		if state == nil {
			continue
		}

		state.current += stackDelta(line)
		if state.current > state.maximum {
			state.maximum = state.current
		}
	}
	flush()
	if err := scanner.Err(); err != nil {
		return nil, err
	}
	return result, nil
}

func stackDelta(line string) int {
	switch {
	case pushInstruction.MatchString(line):
		return 2
	case popInstruction.MatchString(line):
		return -2
	case decdSP.MatchString(line):
		return 2
	case incdSP.MatchString(line):
		return -2
	case decSP.MatchString(line):
		return 1
	case incSP.MatchString(line):
		return -1
	case addSP.MatchString(line):
		value, _ := strconv.Atoi(addSP.FindStringSubmatch(line)[1])
		return -value
	case subSP.MatchString(line):
		value, _ := strconv.Atoi(subSP.FindStringSubmatch(line)[1])
		return value
	default:
		return 0
	}
}

func selectFrame(frames []functionFrame, selector string) (functionFrame, error) {
	var matches []functionFrame
	for _, frame := range frames {
		if strings.Contains(frame.Name, selector) {
			matches = append(matches, frame)
		}
	}
	if len(matches) == 1 {
		return matches[0], nil
	}
	if len(matches) == 0 {
		return functionFrame{}, fmt.Errorf("no matching function")
	}
	identical := true
	for _, match := range matches[1:] {
		if match.Name != matches[0].Name || match.FrameBytes != matches[0].FrameBytes {
			identical = false
			break
		}
	}
	if identical {
		return matches[0], nil
	}

	names := make([]string, 0, len(matches))
	for _, match := range matches {
		names = append(names, fmt.Sprintf("%s (%s)", match.Name, match.Input))
	}
	sort.Strings(names)
	return functionFrame{}, fmt.Errorf("ambiguous; matches %s", strings.Join(names, ", "))
}

func render(path []functionFrame) {
	table := tabwriter.NewWriter(os.Stdout, 0, 4, 2, ' ', 0)
	fmt.Fprintln(table, "FUNCTION\tFRAME\tRETURN\tCUMULATIVE\tINPUT")
	cumulative := 0
	for _, frame := range path {
		cumulative += frame.FrameBytes + msp430ReturnAddressSize
		fmt.Fprintf(table, "%s\t%d\t%d\t%d\t%s\n",
			frame.Name, frame.FrameBytes, msp430ReturnAddressSize, cumulative, frame.Input)
	}
	_ = table.Flush()
	fmt.Printf("\nPeak stack usage for path: %d bytes\n", cumulative)
}

func exitf(format string, arguments ...any) {
	fmt.Fprintf(os.Stderr, "stackusage: "+format+"\n", arguments...)
	os.Exit(2)
}
