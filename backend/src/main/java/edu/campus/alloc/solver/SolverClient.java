package edu.campus.alloc.solver;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import edu.campus.alloc.solver.dto.SolverAllocateResponse;
import edu.campus.alloc.solver.dto.SolverCompareResponse;
import edu.campus.alloc.solver.dto.SolverValidateResponse;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.concurrent.TimeUnit;

@Component
public class SolverClient {

    private final String solverPath;
    private final ObjectMapper objectMapper;

    public SolverClient(@Value("${solver.path:../cpp-engine/build/solver.exe}") String solverPath,
                        ObjectMapper objectMapper) {
        this.solverPath = resolveSolverPath(solverPath);
        this.objectMapper = objectMapper;
    }

    private static String resolveSolverPath(String configuredPath) {
        File f = new File(configuredPath);
        if (f.exists()) {
            return f.getAbsolutePath();
        }
        if (configuredPath.startsWith("../") && new File(configuredPath.substring(3)).exists()) {
            return new File(configuredPath.substring(3)).getAbsolutePath();
        }
        if (!configuredPath.startsWith("../") && new File("../" + configuredPath).exists()) {
            return new File("../" + configuredPath).getAbsolutePath();
        }
        return configuredPath;
    }

    public String getSolverPath() {
        return solverPath;
    }

    public SolverAllocateResponse allocate(String algorithm, String inputContent) {
        String stdout = executeSolver(new String[]{"allocate", algorithm}, inputContent);
        try {
            SolverAllocateResponse response = objectMapper.readValue(stdout, SolverAllocateResponse.class);
            if (!response.isOk()) {
                String errorMsg = response.getError() != null ? response.getError() : "Solver returned ok=false";
                throw new SolverException(errorMsg);
            }
            return response;
        } catch (SolverException se) {
            throw se;
        } catch (Exception e) {
            throw new SolverException("Failed to parse solver allocate output: " + e.getMessage(), e);
        }
    }

    public SolverCompareResponse compare(String inputContent) {
        String stdout = executeSolver(new String[]{"compare"}, inputContent);
        try {
            SolverCompareResponse response = objectMapper.readValue(stdout, SolverCompareResponse.class);
            if (!response.isOk()) {
                String errorMsg = response.getError() != null ? response.getError() : "Solver returned ok=false";
                throw new SolverException(errorMsg);
            }
            return response;
        } catch (SolverException se) {
            throw se;
        } catch (Exception e) {
            throw new SolverException("Failed to parse solver compare output: " + e.getMessage(), e);
        }
    }

    public SolverValidateResponse validate(String inputContent) {
        String stdout = executeSolver(new String[]{"validate"}, inputContent);
        try {
            SolverValidateResponse response = objectMapper.readValue(stdout, SolverValidateResponse.class);
            if (!response.isOk()) {
                String errorMsg = response.getError() != null ? response.getError() : "Solver returned ok=false";
                throw new SolverException(errorMsg);
            }
            return response;
        } catch (SolverException se) {
            throw se;
        } catch (Exception e) {
            throw new SolverException("Failed to parse solver validate output: " + e.getMessage(), e);
        }
    }

    private String executeSolver(String[] subArgs, String inputContent) {
        Path tempFile = null;
        try {
            tempFile = Files.createTempFile("solver_", ".txt");
            Files.writeString(tempFile, inputContent, StandardCharsets.UTF_8);

            String[] cmd = new String[subArgs.length + 2];
            cmd[0] = solverPath;
            System.arraycopy(subArgs, 0, cmd, 1, subArgs.length);
            cmd[cmd.length - 1] = tempFile.toAbsolutePath().toString();

            ProcessBuilder pb = new ProcessBuilder(cmd);
            pb.redirectErrorStream(false);

            Process process;
            try {
                process = pb.start();
            } catch (Exception e) {
                throw new SolverException("Failed to start solver process at '" + solverPath + "': " + e.getMessage(), e);
            }

            StringBuilder stdout = new StringBuilder();
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream(), StandardCharsets.UTF_8))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    stdout.append(line).append("\n");
                }
            }

            boolean finished = process.waitFor(15, TimeUnit.SECONDS);
            if (!finished) {
                process.destroyForcibly();
                throw new SolverException("Solver process timed out after 15 seconds");
            }

            int exitCode = process.exitValue();
            String outputStr = stdout.toString().trim();

            if (exitCode != 0) {
                String errorDetail = outputStr;
                try {
                    JsonNode node = objectMapper.readTree(outputStr);
                    if (node.has("error")) {
                        errorDetail = node.get("error").asText();
                    }
                } catch (Exception ignored) {
                }
                throw new SolverException("Solver exited with code " + exitCode + ": " + errorDetail);
            }

            if (outputStr.isEmpty()) {
                throw new SolverException("Solver produced empty output");
            }

            return outputStr;

        } catch (SolverException se) {
            throw se;
        } catch (Exception e) {
            throw new SolverException("Error during solver execution: " + e.getMessage(), e);
        } finally {
            if (tempFile != null) {
                try {
                    Files.deleteIfExists(tempFile);
                } catch (Exception ignored) {
                }
            }
        }
    }
}
