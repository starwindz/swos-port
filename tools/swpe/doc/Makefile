PDFLATEX = pdflatex
PLTFLAGS =

all : doc_en.pdf doc_sr.pdf

%.pdf : %.tex
	@$(PDFLATEX) $(PLTFLAGS) $<
	@rm -f *.aux *.log

.phony : clean
clean :
	@rm -f *.aux *.log *.pdf