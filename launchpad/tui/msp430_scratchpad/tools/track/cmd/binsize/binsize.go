package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"os"
	"os/exec"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"text/tabwriter"
)

const defaultBuilds = "release=lpmsp430g2553,debug-full=debug-full"

type buildSpec struct {
	Label       string
	Environment string
}

type deviceMeasurement struct {
	MCU       string `json:"mcu"`
	Frequency int64  `json:"frequency"`
	Flash     uint64 `json:"flash"`
	RAM       uint64 `json:"ram"`
}

type sectionMeasurement struct {
	Name      string `json:"name"`
	Size      uint64 `json:"size"`
	StartAddr uint64 `json:"start_addr"`
	Type      string `json:"type"`
	Flags     string `json:"flags"`
	InFlash   bool   `json:"in_flash"`
	InRAM     bool   `json:"in_ram"`
}

type totalMeasurement struct {
	RAMSize   uint64                        `json:"ram_size"`
	FlashSize uint64                        `json:"flash_size"`
	Sections  map[string]sectionMeasurement `json:"sections"`
}

type sizeData struct {
	Device deviceMeasurement `json:"device"`
	Memory struct {
		Total totalMeasurement `json:"total"`
	} `json:"memory"`
}

type measuredBuild struct {
	Label       string                        `json:"label"`
	Environment string                        `json:"environment"`
	Source      string                        `json:"source"`
	FlashUsed   uint64                        `json:"flash_used"`
	RAMUsed     uint64                        `json:"ram_used"`
	Sections    map[string]sectionMeasurement `json:"sections"`
	data        sizeData
}

type comparisonReport struct {
	Device deviceMeasurement `json:"device"`
	Builds []measuredBuild   `json:"builds"`
}

func main() {
	project := flag.String("project", ".", "PlatformIO project directory")
	buildsValue := flag.String("builds", defaultBuilds, "comma-separated label=environment pairs")
	platformIO := flag.String("platformio", "platformio", "PlatformIO executable")
	noBuild := flag.Bool("no-build", false, "read existing sizedata.json files without rebuilding")
	output := flag.String("output", ".pio/build/size-comparison.json", "combined JSON output path; empty disables it")
	flag.Parse()

	if flag.NArg() != 0 {
		exitf("unexpected arguments: %s", strings.Join(flag.Args(), " "))
	}

	specs, err := parseBuildSpecs(*buildsValue)
	if err != nil {
		exitf("parse builds: %v", err)
	}

	projectDir, err := filepath.Abs(*project)
	if err != nil {
		exitf("resolve project directory: %v", err)
	}

	if !*noBuild {
		fmt.Printf("Starting PlatformIO build (%s)...\n", buildLabels(specs))
		if err := runPlatformIO(projectDir, *platformIO, specs); err != nil {
			exitf("build size data: %v", err)
		}
		fmt.Println("PlatformIO build complete.")
		fmt.Println()
	}

	builds, err := readBuilds(projectDir, specs)
	if err != nil {
		exitf("read size data: %v", err)
	}

	if *output != "" {
		outputPath := *output
		if !filepath.IsAbs(outputPath) {
			outputPath = filepath.Join(projectDir, outputPath)
		}
		if err := writeComparison(outputPath, builds); err != nil {
			exitf("write comparison JSON: %v", err)
		}
	}

	if err := renderTable(os.Stdout, builds); err != nil {
		exitf("render table: %v", err)
	}
}

func parseBuildSpecs(value string) ([]buildSpec, error) {
	var specs []buildSpec
	seenLabels := make(map[string]bool)
	seenEnvironments := make(map[string]bool)

	for _, item := range strings.Split(value, ",") {
		label, environment, found := strings.Cut(strings.TrimSpace(item), "=")
		label = strings.TrimSpace(label)
		environment = strings.TrimSpace(environment)
		if !found || label == "" || environment == "" {
			return nil, fmt.Errorf("invalid build %q; expected label=environment", item)
		}
		if seenLabels[label] {
			return nil, fmt.Errorf("duplicate label %q", label)
		}
		if seenEnvironments[environment] {
			return nil, fmt.Errorf("duplicate environment %q", environment)
		}
		seenLabels[label] = true
		seenEnvironments[environment] = true
		specs = append(specs, buildSpec{Label: label, Environment: environment})
	}

	if len(specs) < 2 {
		return nil, fmt.Errorf("at least two builds are required")
	}
	return specs, nil
}

func buildLabels(specs []buildSpec) string {
	labels := make([]string, 0, len(specs))
	for _, spec := range specs {
		labels = append(labels, spec.Label)
	}
	return strings.Join(labels, ", ")
}

func runPlatformIO(projectDir, platformIO string, specs []buildSpec) error {
	args := []string{"run", "--project-dir", projectDir}
	for _, spec := range specs {
		args = append(args, "-e", spec.Environment)
	}
	args = append(args, "--target", "size-breakdown")

	cmd := exec.Command(platformIO, args...)
	cmd.Dir = projectDir
	var output bytes.Buffer
	cmd.Stdout = &output
	cmd.Stderr = &output
	if err := cmd.Run(); err != nil {
		return fmt.Errorf("%s %s: %w\n%s", platformIO, strings.Join(args, " "), err, output.String())
	}
	return nil
}

func readBuilds(projectDir string, specs []buildSpec) ([]measuredBuild, error) {
	builds := make([]measuredBuild, 0, len(specs))
	for _, spec := range specs {
		path := filepath.Join(projectDir, ".pio", "build", spec.Environment, "sizedata.json")
		file, err := os.Open(path)
		if err != nil {
			return nil, fmt.Errorf("open %s: %w", path, err)
		}

		var data sizeData
		decodeErr := json.NewDecoder(file).Decode(&data)
		closeErr := file.Close()
		if decodeErr != nil {
			return nil, fmt.Errorf("decode %s: %w", path, decodeErr)
		}
		if closeErr != nil {
			return nil, fmt.Errorf("close %s: %w", path, closeErr)
		}

		builds = append(builds, measuredBuild{
			Label:       spec.Label,
			Environment: spec.Environment,
			Source:      relativePath(projectDir, path),
			FlashUsed:   data.Memory.Total.FlashSize,
			RAMUsed:     data.Memory.Total.RAMSize,
			Sections:    data.Memory.Total.Sections,
			data:        data,
		})
	}

	base := builds[0].data.Device
	for _, build := range builds[1:] {
		device := build.data.Device
		if device.MCU != base.MCU || device.Flash != base.Flash || device.RAM != base.RAM {
			return nil, fmt.Errorf("build %q targets a different device", build.Label)
		}
	}
	return builds, nil
}

func writeComparison(path string, builds []measuredBuild) error {
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	report := comparisonReport{Device: builds[0].data.Device, Builds: builds}
	contents, err := json.MarshalIndent(report, "", "  ")
	if err != nil {
		return err
	}
	contents = append(contents, '\n')
	return os.WriteFile(path, contents, 0o644)
}

func renderTable(w io.Writer, builds []measuredBuild) error {
	device := builds[0].data.Device
	if _, err := fmt.Fprintf(w, "%s memory comparison\n\n", device.MCU); err != nil {
		return err
	}

	table := tabwriter.NewWriter(w, 0, 4, 2, ' ', 0)
	fmt.Fprint(table, "SECTION\t")
	for _, build := range builds {
		fmt.Fprintf(table, "%s\t", strings.ToUpper(build.Label))
	}
	fmt.Fprintln(table)

	for _, sectionName := range sectionNames(builds) {
		fmt.Fprintf(table, "%s\t", sectionName)
		for _, build := range builds {
			fmt.Fprintf(table, "%s\t", formatBytes(build.Sections[sectionName].Size))
		}
		fmt.Fprintln(table)
	}

	fmt.Fprintln(table)
	writeTotalRow(table, "FLASH USED", builds, func(build measuredBuild) uint64 { return build.FlashUsed })
	writeCapacityRow(table, "FLASH FREE", builds, device.Flash, func(build measuredBuild) uint64 { return build.FlashUsed })
	writePercentRow(table, "FLASH USE", builds, device.Flash, func(build measuredBuild) uint64 { return build.FlashUsed })
	writeTotalRow(table, "RAM USED", builds, func(build measuredBuild) uint64 { return build.RAMUsed })
	writeCapacityRow(table, "RAM FREE", builds, device.RAM, func(build measuredBuild) uint64 { return build.RAMUsed })
	writePercentRow(table, "RAM USE", builds, device.RAM, func(build measuredBuild) uint64 { return build.RAMUsed })

	if err := table.Flush(); err != nil {
		return err
	}

	fmt.Fprintln(w, "\nSources:")
	for _, build := range builds {
		fmt.Fprintf(w, "  %-12s %s\n", build.Label, build.Source)
	}
	return nil
}

func sectionNames(builds []measuredBuild) []string {
	seen := make(map[string]bool)
	for _, build := range builds {
		for name, section := range build.Sections {
			if section.InFlash || section.InRAM {
				seen[name] = true
			}
		}
	}

	preferred := []string{".text", ".rodata", ".data", ".bss", ".noinit", ".vectors"}
	var names []string
	for _, name := range preferred {
		if seen[name] {
			names = append(names, name)
			delete(seen, name)
		}
	}
	var remaining []string
	for name := range seen {
		remaining = append(remaining, name)
	}
	sort.Strings(remaining)
	return append(names, remaining...)
}

func writeTotalRow(w io.Writer, label string, builds []measuredBuild, used func(measuredBuild) uint64) {
	fmt.Fprintf(w, "%s\t", label)
	for _, build := range builds {
		fmt.Fprintf(w, "%s\t", formatBytes(used(build)))
	}
	fmt.Fprintln(w)
}

func writeCapacityRow(w io.Writer, label string, builds []measuredBuild, capacity uint64, used func(measuredBuild) uint64) {
	fmt.Fprintf(w, "%s\t", label)
	for _, build := range builds {
		free := uint64(0)
		if used(build) < capacity {
			free = capacity - used(build)
		}
		fmt.Fprintf(w, "%s\t", formatBytes(free))
	}
	fmt.Fprintln(w)
}

func writePercentRow(w io.Writer, label string, builds []measuredBuild, capacity uint64, used func(measuredBuild) uint64) {
	fmt.Fprintf(w, "%s\t", label)
	for _, build := range builds {
		percent := float64(0)
		if capacity != 0 {
			percent = 100 * float64(used(build)) / float64(capacity)
		}
		fmt.Fprintf(w, "%.1f%%\t", percent)
	}
	fmt.Fprintln(w)
}

func formatBytes(value uint64) string {
	digits := strconv.FormatUint(value, 10)
	for index := len(digits) - 3; index > 0; index -= 3 {
		digits = digits[:index] + "," + digits[index:]
	}
	return digits
}

func relativePath(base, path string) string {
	relative, err := filepath.Rel(base, path)
	if err != nil {
		return path
	}
	return relative
}

func exitf(format string, args ...any) {
	fmt.Fprintf(os.Stderr, "error: "+format+"\n", args...)
	os.Exit(1)
}
