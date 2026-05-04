import logging
import pathlib
import subprocess

logger = logging.getLogger(__name__)

class TestLvr3Reconstruct:

    def test_binary_build(self, buildDirectory):
        filepath = buildDirectory / "lvr2_reconstruct"
        logger.info(f"Testing binary at: {filepath}")
        assert filepath.exists(), f"Binary not found at {filepath}"
        logger.info("Binary exists, attempting to run it...")
        try:
            result = self.run_process([str(filepath)])
            assert "Supported options" in result, "Expected usage information in output"
        except subprocess.CalledProcessError as e:
            logger.error(f"Binary execution failed: {e.stderr}")
            assert False, f"Binary execution failed with error: {e.stderr}"

    def test_filename_parameter(self, buildDirectory):
        filepath = buildDirectory / "lvr2_reconstruct"
        logger.info(f"Testing binary with filename parameter at: {filepath}")
        result = self.run_process([str(filepath), "--inputFile", "tests/data/CrashExample.ply", "--outputFile", "filename_parameter.obj"])
        assert pathlib.Path("filename_parameter.obj").exists(), "Output file was not created"
        logger.info("Output file created successfully.")
        self.deleteFile("filename_parameter.obj")

    def test_optimize_planes_and_fill_holes(self, buildDirectory):
        filepath = buildDirectory / "lvr2_reconstruct"
        logger.info(f"Testing binary with optimize planes and fill holes parameters at: {filepath}")
        filename = "optimize_planes_and_fill_holes.obj"
        datapath = "tests/data/CrashExample.ply"

        # only optimize planes
        result = self.run_process([str(filepath), "--inputFile", datapath, "--outputFile", filename, "-o"])
        assert pathlib.Path(filename).exists(), "Output file was not created"
        logger.info("Output file created successfully.")
        self.deleteFile(filename)

        # only fill holes
        result = self.run_process([str(filepath), "--inputFile", datapath, "--outputFile", filename, "-f", "15"])
        assert pathlib.Path(filename).exists(), "Output file was not created"
        logger.info("Output file created successfully.")
        self.deleteFile(filename)

        # optimize planes and fill holes
        result = self.run_process([str(filepath), "--inputFile", datapath, "--outputFile", filename, "-o", "-f", "15"])
        assert pathlib.Path(filename).exists(), "Output file was not created"
        logger.info("Output file created successfully.")
        self.deleteFile(filename)

    def run_process(self, args):
        try:
            result = subprocess.run(args, capture_output=True, text=True, check=True)
            return result.stdout
        except subprocess.CalledProcessError as e:
            logger.error(f"Process execution failed: {e.stderr}")
            raise

    def deleteFile(self, filename):
        file_path = pathlib.Path(filename)
        if file_path.exists():
            logger.info(f"Deleting file: {filename}")
            file_path.unlink()
        else:
            logger.warning(f"File not found for deletion: {filename}")