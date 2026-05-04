"""
Pytest configuration and shared fixtures.
"""

import pathlib
import pytest


@pytest.fixture(scope="session")
def test_data_dir():
    """Provide path to test data directory."""
    return pathlib.Path(__file__).parent / "data"


@pytest.fixture(scope="function")
def sample_message():
    """Provide a sample message for testing."""
    return "Hello, World!"


@pytest.fixture(scope="function")
def sample_numbers():
    """Provide sample numbers for arithmetic testing."""
    return [1, 2, 3, 4, 5]

@pytest.fixture(scope="function")
def buildDirectory():
    """Provide the repository build directory path."""
    return pathlib.Path(__file__).resolve().parents[1] / "build"