.PHONY: all
all: make-imago make-treestor make-drawtext

.PHONY: clean
clean: clean-imago clean-treestor clean-drawtext

.PHONY: imago
imago: make-imago

.PHONY: make-imago
make-imago:
	cd imago; $(MAKE)

.PHONY: clean-imago
clean-imago:
	cd imago; $(MAKE) clean

.PHONY: treestor
treestor: make-treestor

.PHONY: make-treestor
make-treestor:
	cd treestor; $(MAKE)

.PHONY: clean-treestor
clean-treestor:
	cd treestor; $(MAKE) clean

.PHONY: drawtext
drawtext: make-drawtext

.PHONY: make-drawtext
make-drawtext:
	cd drawtext; $(MAKE)

.PHONY: clean-drawtext
clean-drawtext:
	cd drawtext; $(MAKE) clean
