OUT_DIR=build
SHADER_SOURCES := $(wildcard *.vert *.frag *.geom)
SHADER_INCLUDES := $(wildcard *.glsl)
SPVS = $(addprefix $(OUT_DIR)/, $(SHADER_SOURCES:=.spv))

all: $(OUT_DIR) $(SPVS)

$(OUT_DIR):
	mkdir -p $@

$(OUT_DIR)/%.spv: % $(SHADER_INCLUDES)
	glslc -O $< -o $@

clean:
	rm -r $(OUT_DIR)
