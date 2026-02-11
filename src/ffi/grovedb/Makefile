.PHONY: docs coverage-fuzz coverage-test

coverage-fuzz:
	mkdir -p contrib/gcovr/output/fuzz
	gcovr --config contrib/gcovr/fuzz.cfg

coverage-test:
	mkdir -p contrib/gcovr/output/test
	gcovr --config contrib/gcovr/test.cfg

docs:
	python3 contrib/build_docs.py
