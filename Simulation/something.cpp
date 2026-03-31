//#include <iostream>
//#include <ceres/ceres.h>
//#include <Eigen/Core>
//#include <Eigen/Geometry>
//
//// 1. Define the Cost Function (Reprojection Error)
//struct ReprojectionError {
//    ReprojectionError(double observed_x, double observed_y)
//        : observed_x(observed_x), observed_y(observed_y) {
//    }
//
//    template <typename T>
//    bool operator()(const T* const camera, const T* const point, T* residuals) const {
//        // camera[0,1,2] is rotation (Angle-axis), [3,4,5] is translation
//        T p[3];
//        ceres::AngleAxisRotatePoint(camera, point, p);
//        p[0] += camera[3];
//        p[1] += camera[4];
//        p[2] += camera[5];
//
//        // Simple pinhole projection (focal length = 1.0, no distortion)
//        T xp = p[0] / p[2];
//        T yp = p[1] / p[2];
//
//        // Residual = Observed - Predicted
//        residuals[0] = T(observed_x) - xp;
//        residuals[1] = T(observed_y) - yp;
//
//        return true;
//    }
//
//    double observed_x, observed_y;
//};
//
//int main() {
//    // 2. Initial Data (Camera: 0 rot, 10 units back; Point: near origin)
//    double camera[6] = { 0, 0, 0, 0, 0, 10 };
//    double point[3] = { 0.2, -0.1, 0.5 };
//    double observed_x = 0.05, observed_y = -0.02;
//
//    // 3. Build the Optimization Problem
//    ceres::Problem problem;
//    ceres::CostFunction* cost_function =
//        new ceres::AutoDiffCostFunction<ReprojectionError, 2, 6, 3>(
//            new ReprojectionError(observed_x, observed_y));
//
//    problem.AddResidualBlock(cost_function, nullptr, camera, point);
//
//    // 4. Configure and Run Solver
//    ceres::Solver::Options options;
//    options.linear_solver_type = ceres::DENSE_SCHUR; // Good for BA
//    options.minimizer_progress_to_stdout = true;
//
//    ceres::Solver::Summary summary;
//    ceres::Solve(options, &problem, &summary);
//
//    std::cout << summary.FullReport() << "\n";
//    std::cout << "Optimized Point: " << point[0] << ", " << point[1] << ", " << point[2] << "\n";
//
//    return 0;
//}


//#include <opencv2/opencv.hpp>
//#include <iostream>
//#include <string>
//#include <iomanip>
//#include <cmath>
//
//int main()
//{
//    cv::Mat flowerMap = cv::imread("flower_pipe.jpg");
//
//    if (flowerMap.empty())
//    {
//        std::cout << "Error: flower_pipe.jpg not found!" << std::endl;
//        return -1;
//    }
//
//    // --- 1. Pipe & Camera Parameters ---
//    const double pipeRadius = 150.0;  // mm
//    const double pipeLength = 2000.0; // mm (Texture repeat length)
//    const double fx = 600.0, fy = 600.0, cx = 320.0, cy = 240.0;
//    const double x_off = 20.0; // mm to the right
//    const double y_off = 60.0; // mm down (hugging the bottom)
//    const double pitch = 0.05; // Tilt up/down
//    const double yaw = 0.03; // Tilt left/right
//    const double roll = 0.02; // Rotation around the lens axis
//
//    // Rotation around X (Pitch)
//    cv::Mat R_x = (cv::Mat_<double>(3, 3) <<
//        1, 0, 0,
//        0, cos(pitch), -sin(pitch),
//        0, sin(pitch), cos(pitch));
//
//    // Rotation around Y (Yaw)
//    cv::Mat R_y = (cv::Mat_<double>(3, 3) <<
//        cos(yaw), 0, sin(yaw),
//        0, 1, 0,
//        -sin(yaw), 0, cos(yaw));
//
//    // Rotation around Z (Roll)
//    cv::Mat R_z = (cv::Mat_<double>(3, 3) <<
//        cos(roll), -sin(roll), 0,
//        sin(roll), cos(roll), 0,
//        0, 0, 1);
//
//    cv::Mat R_combined = R_z * R_y * R_x;
//
//    // Lens Distortion: k1 > 0 for Barrel, k1 < 0 for Pincushion
//    const double k1 = 0.2;
//
//    // Simulation Settings
//    const int totalFrames = 100;
//    const double speed = 15.0; // mm per frame
//
//    for (int frame = 0; frame < totalFrames; frame++)
//    {
//        // GLOBAL FRAME: Camera starts at Z=0 and moves into the pipe
//        double cameraGlobalZ = frame * speed;
//
//        cv::Mat cameraView = cv::Mat::zeros(480, 640, CV_8UC3);
//        cv::Mat lightMask = cv::Mat::zeros(cameraView.size(), CV_32F);
//
//        for (int v = 0; v < cameraView.rows; v++)
//        {
//            for (int u = 0; u < cameraView.cols; u++)
//            {
//
//                // --- STEP A: Ray Generation with Tilt ---
//                double dx = (u - cx) / fx;
//                double dy = (v - cy) / fy;
//
//                // Apply distortion if you still want it
//                double r2 = dx * dx + dy * dy;
//                double distort = 1.0 + k1 * r2;
//                dx /= distort;
//                dy /= distort;
//
//                // Create the ray vector
//                cv::Mat ray_local = (cv::Mat_<double>(3, 1) << dx, dy, 1.0);
//
//                // ROTATE the ray based on camera tilt
//                cv::Mat ray_rotated = R_combined * ray_local;
//
//                double rx = ray_rotated.at<double>(0);
//                double ry = ray_rotated.at<double>(1);
//                double rz = ray_rotated.at<double>(2);
//
//                //--
//
//                // --- STEP B: Intersection with rotated ray ---
//                double A = rx * rx + ry * ry;
//                double B = 2.0 * (rx * x_off + ry * y_off);
//                double C = x_off * x_off + y_off * y_off - pipeRadius * pipeRadius;
//
//                double disc = B * B - 4.0 * A * C;
//                double t = (-B + std::sqrt(disc)) / (2.0 * A);
//
//                // The point on the wall in the camera's original coordinate system
//                double wallX = x_off + t * rx;
//                double wallY = y_off + t * ry;
//                double localZ = t * rz; // The depth into the pipe
//
//                // --- STEP C: Global Mapping ---
//                double globalPointZ = cameraGlobalZ + localZ;
//                double theta = std::atan2(wallY, wallX);
//                if (theta < 0) theta += 2.0 * CV_PI;
//
//                // Map to texture coordinates with wrap-around
//                int srcX = static_cast<int>((theta / (2.0 * CV_PI)) * flowerMap.cols);
//                int srcY = static_cast<int>(std::fmod(globalPointZ, pipeLength) / pipeLength * flowerMap.rows);
//
//                // Sample texture
//                if (srcX >= 0 && srcX < flowerMap.cols && srcY >= 0 && srcY < flowerMap.rows)
//                {
//                    cameraView.at<cv::Vec3b>(v, u) = flowerMap.at<cv::Vec3b>(srcY, srcX);
//                }
//
//                // --- STEP D: Lighting (Always relative to camera distance) ---
//                double attenuation = 700.0 / (localZ + 200.0);
//                lightMask.at<float>(v, u) = std::fmin(1.0f, std::fmax(0.1f, (float)attenuation));
//            }
//        }
//
//        // --- 2. Post-Processing ---
//
//        // Apply Lighting
//        cv::Mat cameraViewFloat;
//        cameraView.convertTo(cameraViewFloat, CV_32FC3);
//        std::vector<cv::Mat> channels;
//        cv::split(cameraViewFloat, channels);
//        for (int i = 0; i < 3; i++) channels[i] = channels[i].mul(lightMask);
//        cv::merge(channels, cameraViewFloat);
//        cameraViewFloat.convertTo(cameraView, CV_8UC3);
//
//        // Add Sensor Noise
//        cv::Mat noise = cv::Mat::zeros(cameraView.size(), cameraView.type());
//        cv::randn(noise, cv::Scalar(0, 0, 0), cv::Scalar(10, 10, 10));
//        cameraView += noise;
//
//        // --- 3. Save & Display ---
//        std::stringstream ss;
//        ss << "flower_pipe_sims//pipe_frame_" << std::setfill('0') << std::setw(4) << frame << ".jpg";
//        cv::imwrite(ss.str(), cameraView);
//
//        cv::imshow("Global Frame Pipe Simulator", cameraView);
//        if (cv::waitKey(1) == 27) break; // ESC to quit
//    }
//
//    return 0;
//}