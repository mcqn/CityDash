# Installation

## Initial Configuration

Set up the configuration
    `cp config.example.yaml config.yaml`
    `vi config.yaml`

## Running Manually

    `ruby citydash.rb config.yaml`

### Running as a Docker Container

1. Build the container
    `docker build -t mcqnltd/citydash .`

1. Run the container
    `docker run --name citydash --rm -t mcqnltd/citydash`

1. To run under systemd
    `sudo cp citydash.service /lib/systemd/system/`
    `sudo systemctl enable citydash.service`
    `sudo systemctl daemon-reload`
    `sudo service citydash start`
