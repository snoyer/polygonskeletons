
build = generated
examples = $(patsubst %.txt, %, $(wildcard example*.txt))
pdfs = $(patsubst %, $(build)/%.pdf, $(examples))
pngs = $(patsubst %, $(build)/%.png, $(examples))

all: $(pdfs) $(pngs)

$(build)/%.pdf: %.txt
	@mkdir -p $(@D)
	python example-figure.py $< $@

$(build)/%.png: $(build)/%.pdf
		convert -density 150 $< $@
