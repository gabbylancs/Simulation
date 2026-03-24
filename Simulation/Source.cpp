#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <iomanip> // for std::setw

int main() {
    cv::Mat flowerMap = cv::imread("flower_pipe.jpg");
    if (flowerMap.empty()) return -1;

    // Constants
    double pipeRadius = 150.0;
    double pipeLength = 2000.0;
    double fx = 600.0, fy = 600.0, cx = 320.0, cy = 240.0;

    // Animation Settings
    int totalFrames = 50;
    double startZ = 500.0; // Starting distance
    double endZ = 100.0;   // Ending distance (moving deeper into the pipe)

    for (int frame = 0; frame < totalFrames; frame++) {
        // Calculate current Z position for this frame
        double currentZOffset = startZ - ((startZ - endZ) * (double)frame / totalFrames);

        cv::Mat cameraView = cv::Mat::zeros(480, 640, CV_8UC3);

        // --- Core Projection Loop ---
        for (int v = 0; v < cameraView.rows; v++) {
            for (int u = 0; u < cameraView.cols; u++) {
                double normX = (u - cx) / fx;
                double normY = (v - cy) / fy;

                double t = pipeRadius / std::sqrt(normX * normX + normY * normY);
                double Z = t * 1.0;

                double theta = std::atan2(t * normY, t * normX);
                if (theta < 0) theta += 2.0 * CV_PI;

                // Use the moving currentZOffset here
                double z_in_pipe = Z - currentZOffset;

                int srcX = static_cast<int>((theta / (2.0 * CV_PI)) * flowerMap.cols);
                int srcY = static_cast<int>((z_in_pipe / pipeLength) * flowerMap.rows);

                if (srcX >= 0 && srcX < flowerMap.cols && srcY >= 0 && srcY < flowerMap.rows) {
                    cameraView.at<cv::Vec3b>(v, u) = flowerMap.at<cv::Vec3b>(srcY, srcX);
                }
            }
        }

        // --- Apply Effects (Noise/Vignette/Flashlight) ---
        // (Insert your previous noise and vignette code here)

        // --- Save Frame ---
        std::stringstream ss;
        ss << "frame_" << std::setfill('0') << std::setw(3) << frame << ".jpg";
        cv::imwrite(ss.str(), cameraView);

        std::cout << "Saved: " << ss.str() << " (Z: " << currentZOffset << ")" << std::endl;

        // Optional: Show progress
        cv::imshow("Simulating...", cameraView);
        if (cv::waitKey(1) == 27) break; // Exit on ESC
    }

    return 0;
}

//#include <opencv2/opencv.hpp>
//#include <iostream>
//#include <cmath>
//
//void addSaltAndPepper(cv::Mat& image, float noise_fraction)
//{
//    int amount = static_cast<int>(image.rows * image.cols * noise_fraction);
//    for (int i = 0; i < amount; i++)
//    {
//        int r = rand() % image.rows;
//        int c = rand() % image.cols;
//        // Randomly choose between white (255) and black (0)
//        image.at<cv::Vec3b>(r, c) = (rand() % 2 == 0) ? cv::Vec3b(255, 255, 255) : cv::Vec3b(0, 0, 0);
//    }
//}
//
//int main() {
//    // 1. Load the unwrapped pipe image
//    // Assume: Width = 0 to 360 degrees, Height = 0 to Max Depth
//    cv::Mat flowerMap = cv::imread("flower_pipe.jpg");
//    if (flowerMap.empty()) {
//        std::pair<int, int> error; // Just a placeholder for your error handling
//        std::cout << "Could not open or find the image 'flower_pipe.jpg'" << std::endl;
//        return -1;
//    }
//
//    // 2. Physical & Camera Parameters
//    double pipeRadius = 150.0;    // mm
//    double pipeLength = 2000.0;   // mm
//    double zOffset = 200.0;       // Distance from camera to start of pipe
//
//    // Camera Intrinsic Matrix K
//    double fx = 600.0, fy = 600.0;
//    double cx = 320.0, cy = 240.0;
//
//    // 3. Create Output Image
//    cv::Mat cameraView = cv::Mat::zeros(480, 640, CV_8UC3);
//
//    // 4. Backward Mapping (Loop over output pixels)
//    for (int v = 0; v < cameraView.rows; v++) {
//        for (int u = 0; u < cameraView.cols; u++) {
//
//            // Step A: Convert pixel (u,v) to normalized camera coordinates (x, y, 1)
//            double normX = (u - cx) / fx;
//            double normY = (v - cy) / fy;
//
//            /* Step B: Intersection with Cylinder.
//               A ray from origin (0,0,0) through (normX, normY, 1) is:
//               P = t * [normX, normY, 1]
//               Cylinder equation: P.x^2 + P.y^2 = R^2
//               (t*normX)^2 + (t*normY)^2 = R^2
//            */
//            double t = pipeRadius / std::sqrt(normX * normX + normY * normY);
//
//            // 3D Point on cylinder wall
//            double X = t * normX;
//            double Y = t * normY;
//            double Z = t * 1.0; // depth
//
//            // Step C: Map 3D back to Unwrapped Image Coordinates
//            // 1. Get Angle (theta)
//            double theta = std::atan2(Y, X); // -PI to PI
//            if (theta < 0) theta += 2.0 * CV_PI; // 0 to 2*PI
//
//            // 2. Get Depth (z)
//            double z_in_pipe = Z - zOffset;
//
//            // Step D: Convert Theta/Z to Source Pixel indices
//            int srcX = static_cast<int>((theta / (2.0 * CV_PI)) * flowerMap.cols);
//            int srcY = static_cast<int>((z_in_pipe / pipeLength) * flowerMap.rows);
//
//            // Step E: Boundary Check and Sample
//            if (srcX >= 0 && srcX < flowerMap.cols && srcY >= 0 && srcY < flowerMap.rows) {
//                cameraView.at<cv::Vec3b>(v, u) = flowerMap.at<cv::Vec3b>(srcY, srcX);
//            }
//        }
//    }
//    
//    // --- ADDING NOISE SECTION ---
//
//    // 1. Add Gaussian Noise
//    cv::Mat gaussianNoise = cv::Mat::zeros(cameraView.size(), cameraView.type());
//    // Mean 0, StdDev 15 (subtle grain)
//    cv::randn(gaussianNoise, cv::Scalar(0, 0, 0), cv::Scalar(15, 15, 15));
//    cameraView += gaussianNoise;
//
//    // 2. Add Salt & Pepper (optional)
//    // 0.01 = 1% of pixels are noise
//    int sp_count = static_cast<int>(cameraView.rows * cameraView.cols * 0.005);
//    for (int k = 0; k < sp_count; k++) {
//        int i = rand() % cameraView.rows;
//        int j = rand() % cameraView.cols;
//        cameraView.at<cv::Vec3b>(i, j) = (rand() % 2 == 0) ? cv::Vec3b(255, 255, 255) : cv::Vec3b(0, 0, 0);
//    }
//
//    // 3. Final Step: Slight Blur
//    // Real cameras aren't perfectly sharp; a tiny blur makes the noise look more realistic
//    cv::GaussianBlur(cameraView, cameraView, cv::Size(3, 3), 0.5);
//
//    // --- 3. APPLY LED LIGHT FALL-OFF (Flashlight Effect) ---
//    // We'll create a new lighting mask based on Depth (Z)
//    cv::Mat lightMask = cv::Mat::zeros(cameraView.size(), CV_32F);
//
//    // We need to re-run a simplified loop or save Z values to apply lighting correctly
//    for (int v = 0; v < cameraView.rows; v++) {
//        for (int u = 0; u < cameraView.cols; u++) {
//            double normX = (u - cx) / fx;
//            double normY = (v - cy) / fy;
//
//            // Re-calculate Z for this pixel
//            double t = pipeRadius / std::sqrt(normX * normX + normY * normY);
//            double Z = t * 1.0;
//
//            // Light fall-off formula: 1 / (Z^2) is realistic, 
//            // but we'll use a tunable linear/exp fall-off for the "look"
//            double attenuation = 500.0 / (Z + 100.0); // Adjust '500' to change brightness
//            if (attenuation > 1.0) attenuation = 1.0;
//            if (attenuation < 0.1) attenuation = 0.1; // Ambient light
//
//            lightMask.at<float>(v, u) = static_cast<float>(attenuation);
//        }
//    }
//
//    // Apply the light mask to the image
//    cv::Mat cameraViewFloat;
//    cameraView.convertTo(cameraViewFloat, CV_32FC3);
//    std::vector<cv::Mat> channels;
//    cv::split(cameraViewFloat, channels);
//    for (int i = 0; i < 3; i++) {
//        channels[i] = channels[i].mul(lightMask);
//    }
//    cv::merge(channels, cameraViewFloat);
//    cameraViewFloat.convertTo(cameraView, CV_8UC3);
//
//    // --- 4. SHOW RESULTS ---
//    cv::imshow("Realistic Pipe View", cameraView);
//    cv::waitKey(0);
//
//    return 0;
}