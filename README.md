<p align="center">
<!---<img src="assets/logos/128x128.png">-->
 <h1 align="center">Docker and VA Smalltalk Examples</h1>
  <p align="center">
    Running VA Smalltalk on Docker containers!
    <br>
    <a href="docs/"><strong>Explore the docs »</strong></a>
    <br>
    <br>
    <a href="https://github.com/vasmalltalk/docker-examples/issues/new?labels=Type%3A+Defect">Report a defect</a>
    |
    <a href="https://github.com/vasmalltalk/docker-examples/issues/new?labels=Type%3A+Feature">Request feature</a>
  </p>
</p>

<!---
[![GitHub release](https://img.shields.io/github/release/vasmalltalk/docker-examples.svg)](https://github.com/vasmalltalk/docker-examples/releases/latest)
[![Build Status](https://travis-ci.com/vasmalltalk/docker-examples.svg?branch=release-candidate)](https://travis-ci.com/vasmalltalk/docker-examples)
[![Coverage Status](https://coveralls.io/repos/github/vasmalltalk/docker-examples/badge.svg?branch=release-candidate)](https://coveralls.io/github/vasmalltalk/docker-examples?branch=release-candidate)
-->

This project serves as a public place where VA Smalltalk users can see and submit Docker examples. Ideally we would like to have many examples showing different OS alternatives and customizations.


## License
- The code is licensed under [MIT](LICENSE).
- The documentation is licensed under [CC BY-SA 4.0](http://creativecommons.org/licenses/by-sa/4.0/).

## Quick Start

- Install `Docker` in the machine you want to run the containers
- Clone this repository
- Pick of the provided examples.
- Run Docker `build` and `run` commands


## Next steps

There are lots of areas of interest we would like to investigate and research. But a few possibilities are:

- Make VA run with docker more images:
  - A regular CentOS/RedHat/Fedora variant.
  - Alipe Linux and Fedora Atomic
- Try with [balenaOS](https://www.balena.io/os/) as a host OS and a [pigpio-ready image](https://github.com/lachatak/rpi-pigpio). This way we can also test the [VAST GPIO binding](http://vastgoodies.com/projects/Raspberry%2520Pi%2520Hardware%2520Interface)
- Take a look to [portainer.io](https://www.portainer.io/) container for graphical management.
- Use `docker-compose` and allow having N number of replicas (our VAST running images) running in the same node with a load balancer on front.
- Experiment with `Docker Swarm` and `Kubernetes`


## Acknowledgments

- [Norbert Schlemmer](https://github.com/Noschvie) for the work on getting the examples to work in Raspberry.
- [Julian Maestri](https://github.com/serpi90) for the initial Dockerfile examples for VA Smalltalk and general help.
- Github repository layout was generated with [Ba-St Github-setup project](https://github.com/ba-st/GitHub-setup).


## Contributing

Check the [Contribution Guidelines](CONTRIBUTING.md)
