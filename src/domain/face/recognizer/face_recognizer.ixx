export module domain.face.recognizer;

export import :api;
export import :factory;
export import :impl.arcface; // Optional: export impl if needed, but usually factory is enough.
// Actually, usually we don't export implementation details directly unless needed for testing or
// extension. But `face_detector.ixx` doesn't seem to export impls, just internal creators. Let's
// stick to API and Factory.

// However, I need to make sure :impl.arcface is compiled. It is imported by factory.cpp, so it
// should be fine. But if I want to use ArcFace class directly in tests, I might need to export it
// or include it. The factory returns unique_ptr<FaceRecognizer>, so users interact via interface.
