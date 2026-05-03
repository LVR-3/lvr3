"""Integration test examples."""

import pytest
import logging

logger = logging.getLogger(__name__)


class TestIntegration:
    """Integration test examples."""

    def test_message_fixture_integration(self, sample_message):
        """Test using shared fixtures."""
        assert isinstance(sample_message, str)
        assert sample_message.startswith("Hello")

    def test_multiple_fixtures(self, sample_message, sample_numbers):
        """Test combining multiple fixtures."""
        msg_len = len(sample_message)
        numbers_count = len(sample_numbers)

        logger.info(f"Message: '{sample_message}' (length: {msg_len})")
        logger.info(f"Numbers: {sample_numbers} (count: {numbers_count})")

        assert msg_len > 0
        assert numbers_count > 0
        assert msg_len + numbers_count > 0
