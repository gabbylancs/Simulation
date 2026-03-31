//int main()
//{
//    const double pipeRadius = 67.42; //mm
//    const double fx = 2608.721575, fy = 2584.771277, cx = 1493.821507, cy = 1191.874317;
//    //const double fx = 600.0, fy = 600.0, cx = 320.0, cy = 240.0;
//
//    cv::Mat img1 = cv::imread("flower_pipe_sims_1/pipe_frame_0001.jpg");
//    cv::Mat img2 = cv::imread("flower_pipe_sims_1/pipe_frame_0007.jpg");
//
//    if (img1.empty() || img2.empty()) return -1;
//
//    // --- 1. PREPROCESSING ---
//    auto preprocess = [](cv::Mat& img) {
//        cv::Mat gray;
//        cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
//
//        // CLAHE to handle lighting in dark pipes
//        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
//        clahe->apply(gray, gray);
//
//        // Slight Blur to reduce sensor noise
//        cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0);
//        return gray;
//        };
//
//    cv::Mat gray1 = preprocess(img1);
//    cv::Mat gray2 = preprocess(img2);
//
//    // --- 2. FEATURE DETECTION ---
//    cv::Ptr<cv::ORB> orb = cv::ORB::create(2000); // Increased for better RANSAC pool
//    std::vector<cv::KeyPoint> kp1, kp2;
//    cv::Mat desc1, desc2;
//    orb->detectAndCompute(gray1, cv::noArray(), kp1, desc1);
//    orb->detectAndCompute(gray2, cv::noArray(), kp2, desc2);
//
//    // --- 3. MATCHING ---
//    cv::BFMatcher matcher(cv::NORM_HAMMING);
//    std::vector<cv::DMatch> matches;
//    matcher.match(desc1, desc2, matches);
//
//    // --- 4. RANSAC FILTERING ---
//    std::vector<cv::Point2f> pts1, pts2;
//    for (const auto& m : matches) {
//        pts1.push_back(kp1[m.queryIdx].pt);
//        pts2.push_back(kp2[m.trainIdx].pt);
//    }
//
//    // Find Fundamental Matrix with RANSAC to identify inliers
//    std::vector<uchar> inliersMask;
//    cv::findFundamentalMat(pts1, pts2, cv::FM_RANSAC, 3.0, 0.99, inliersMask);
//
//    std::vector<cv::DMatch> ransacMatches;
//    for (size_t i = 0; i < inliersMask.size(); i++) {
//        if (inliersMask[i]) {
//            ransacMatches.push_back(matches[i]);
//        }
//    }
//
//    cv::Mat inliersMask2;
//    cv::Mat K = (cv::Mat_<double>(3, 3) << fx, 0, cx, 0, fy, cy, 0, 0, 1);
//    cv::Mat E = cv::findEssentialMat(pts1, pts2, K, cv::RANSAC, 0.999, 1.0, inliersMask2);
//    cv::Mat R, t;
//    cv::recoverPose(E, pts1, pts2, K, R, t, inliersMask2);
//
//    // --- 5. VISUALIZATION ---
//    cv::Mat imgMatches;
//    cv::hconcat(img1, img2, imgMatches);
//
//    for (const auto& m : ransacMatches)
//    {
//        cv::Point2f pt1 = kp1[m.queryIdx].pt;
//        cv::Point2f pt2 = kp2[m.trainIdx].pt;
//        cv::Point2f pt2_shifted = pt2 + cv::Point2f((float)img1.cols, 0.0f);
//
//        cv::line(imgMatches, pt1, pt2_shifted, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);
//    }
//
//    // --- NEW: DRAW EPIPOLE ---
//    if (!t.empty()) {
//        double tx = t.at<double>(0), ty = t.at<double>(1), tz = t.at<double>(2);
//        if (std::abs(tz) > 0.01) { // Forward motion check
//            cv::Point2f epipole(static_cast<float>((fx * tx / tz) + cx),
//                static_cast<float>((fy * ty / tz) + cy));
//
//            cv::Point2f epShifted = epipole + cv::Point2f((float)img1.cols, 0);
//            cv::drawMarker(imgMatches, epShifted, cv::Scalar(0, 0, 255), cv::MARKER_CROSS, 40, 3);
//        }
//    }
//
//
//    // Resize the final combined image for display (e.g., 50% scale)
//    cv::Mat displayImg;
//    double scale = 0.8;
//    cv::resize(imgMatches, displayImg, cv::Size(), scale, scale, cv::INTER_LINEAR);
//
//    cv::imshow("RANSAC Inlier Matches (Resized for Display)", displayImg);
//    cv::waitKey(0);
//
//    return 0;
//}