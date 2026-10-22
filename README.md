# Industrial Sorting System — C++ Multithreading

**Academic Engineering Project | Universidad Europea de Madrid | 2026**

## Overview

Development of an industrial sorting simulation using C++ and the Windows Win32 API.

The application simulates two conveyor belts supplying parts to two robotic sorting stations. Five concurrent threads coordinate production, classification and system monitoring.

## Technologies

- C++ and Win32 API
- Multithreading with `CreateThread`
- Mutexes and semaphores
- Shared circular buffer with 8 positions
- File input/output
- Production statistics and monitoring

## System Architecture

The application consists of:

- **2 producer threads:** simulate conveyor belts.
- **2 consumer threads:** simulate robots sorting parts.
- **1 monitoring thread:** displays production statistics every five seconds.

Parts are classified into three categories: A, B and C.

## Test Results

The documented test included 28 input parts. The generated output files contain 28 correctly classified records, with no missing or duplicated identifiers in that test.

The application is a software simulation; no physical conveyor belts or robots were involved.

## Project Team

- César Malitaxi Martínez
- Ignacio Barreiro San Martín
- Luka Franjo Kurtovic Shiguiyama

**Course:** Industrial Computing — Universidad Europea de Madrid.

## Project Status

Academic implementation completed. Source code review and documentation improvements are in progress.
