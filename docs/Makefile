.PHONY: install runserver

BASEURL := /libreloaded
# The site Jekyll serves: the one scripts/make-docs-site assembles (make docs-server passes it)
SITE ?= $(abspath $(CURDIR)/../build-docs/site)
# Without Ruby on the host Jekyll runs in the official Ruby image. The gems live in a named
# volume, so they are installed once, and the site is built inside the container rather than into
# the site's directory, which would otherwise fill up with files owned by root.
DOCKER_RUBY_IMAGE := ruby:3.3
DOCKER_GEMS_VOLUME := maszyna-docs-gems
DOCKER_RUN = docker run --rm -it -v "$(SITE):/srv/site" -v $(DOCKER_GEMS_VOLUME):/usr/local/bundle \
	-w /srv/site -p 4000:4000 -p 35729:35729 $(DOCKER_RUBY_IMAGE)

ifneq ($(shell command -v bundle),)

install:
	bundle install

runserver: install
	cd "$(SITE)" && bundle exec jekyll serve --livereload --baseurl "$(BASEURL)"

else

install:
	docker run --rm -it -v "$(CURDIR):/srv/docs" -v $(DOCKER_GEMS_VOLUME):/usr/local/bundle -w /srv/docs \
		$(DOCKER_RUBY_IMAGE) bundle install

runserver:
	$(DOCKER_RUN) sh -c 'bundle install && bundle exec jekyll serve --host 0.0.0.0 --livereload \
		--baseurl "$(BASEURL)" --destination /tmp/site'

endif
