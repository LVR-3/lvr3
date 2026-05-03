"""
Pytest configuration and shared fixtures.
"""

import pytest


@pytest.fixture(scope="session")
def test_data_dir():
    """Provide path to test data directory."""
    import pathlib
    return pathlib.Path(__file__).parent / "data"


@pytest.fixture(scope="function")
def sample_message():
    """Provide a sample message for testing."""
    return "Hello, World!"


@pytest.fixture(scope="function")
def sample_numbers():
    """Provide sample numbers for arithmetic testing."""
    return [1, 2, 3, 4, 5]
