# Container build file for the @merseyshipping twitter bot
FROM ruby:2.1
MAINTAINER adrianm@mcqn.com

# Install into /var/local/merseyshipping
RUN mkdir /var/local/citydash
WORKDIR /var/local/citydash

# Install prerequisites
ADD Gemfile .
RUN bundle install

# Copy the scripts over
ADD citydash.rb .
ADD config.yaml .

# And kick things off...
CMD ruby /var/local/citydash/citydash.rb config.yaml
