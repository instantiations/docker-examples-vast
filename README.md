<p align="center">
<!---<img src="assets/logos/128x128.png">-->
 <h1 align="center">Docker and VA Smalltalk Examples</h1>
  <p align="center">
    Running VA Smalltalk on Docker containers!
    <!---
    <br>
    <a href="docs/"><strong>Explore the docs »</strong></a>
    <br>
    -->
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

This project serves as a public place where VA Smalltalk users can see and submit Docker examples. Ideally we would like to have many examples showing different operating systems, CPU architectures, alternatives and customizations.


## License
- The code is licensed under [MIT](LICENSE).
- The documentation is licensed under [CC BY-SA 4.0](http://creativecommons.org/licenses/by-sa/4.0/).

## Quick Start

- Install `Docker` in the machine you want to run the containers
- Clone this repository
- Pick of the provided examples.
- Run the Docker `build` and `run` commands that can be found in each `Dockerfile`
- Read blog posts:
  - [Getting started with Docker and Smalltalk!](https://dev.to/martinezpeck/getting-started-with-docker-and-smalltalk-4po1)
  - [Step 2: Single-node Docker Swarm and Smalltalk](https://dev.to/martinezpeck/step-2-single-node-docker-swarm-and-smalltalk-46i0)
  - [Docker Swarm cloud on a ARM64 DIY SBC cluster running a Smalltalk webapp](https://dev.to/martinezpeck/docker-swarm-cloud-on-a-arm64-diy-sbc-cluster-running-a-smalltalk-webapp-9l1)
  - [Deploying VASmalltalk on Amazon AWS ARM servers](https://dev.to/martinezpeck/deploying-vasmalltalk-on-amazon-aws-arm-servers-aan)
- Watch presentations and videos:
  - ["Improving VASmalltalk deployment, availability and scalability with Docker" at ESUG 2019](https://youtu.be/phQnG4wX9j0)
  - ["Improving VASmalltalk deployment, availability and scalability with Docker" at Smalltalks 2019](https://youtu.be/XtwWQ75VmkM)


## Experiments already run

- Use `docker-compose` and allow having N number of replicas (our VAST images) running in the same node with a load balancer on front.
- Use `Docker Swarm` to create our own cloud and run N number of VAST images distributes in M number of nodes.
- Run Docker in ARM 32 bits (Raspberry Pi 3B+ and Raspbian) and ARM 64 bits (Raspberry Pi 3B+ / Rock64 with Ubuntu Server / Armbian).
- Take a look to [portainer.io](https://www.portainer.io/) container for graphical management.
- [Experiment with Alpine Linux](https://github.com/vasmalltalk/docker-examples/blob/master/source/SeasideTrafficLights/Raspberry/experiments/alpine-raspbian_Dockerfile). Results were not as good as expected because Alpine uses `musl` instead of `glibc`. If you install `glibc` then the docker image gets much bigger and quite similar to a `debian slim`. In addition, it's `glibc` does not seem to work correctly on ARM.

## Next steps

There are lots of areas of interest we would like to investigate and research. But a few possibilities are:

- Make VA run with more different operating systems:
  - A regular CentOS/RedHat/Fedora variant.
  - Fedora Atomic
- Try with [balenaOS](https://www.balena.io/os/) as a host OS and a [pigpio-ready image](https://github.com/lachatak/rpi-pigpio).
- Test our [VAST GPIO binding](http://vastgoodies.com/projects/Raspberry%2520Pi%2520Hardware%2520Interface) from within a container.
- Even if not exactly related to Docker, we would also like to adapt current examples for Kubernetes and OpenShift.


## Acknowledgments

- [Norbert Schlemmer](https://github.com/Noschvie) for the work on getting the examples to work in Raspberry.
- [Julian Maestri](https://github.com/serpi90) for the initial Dockerfile examples for VA Smalltalk and general help.
- Github repository layout was generated with [Ba-St Github-setup project](https://github.com/ba-st/GitHub-setup).


## Contributing

Check the [Contribution Guidelines](CONTRIBUTING.md)
